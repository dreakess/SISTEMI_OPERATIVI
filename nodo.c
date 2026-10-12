#include "lib.h"

#define USER "node"

struct transaction_pool{
	int i; /* Id progressivo */
	struct transazione transazioni[SO_BLOCK_SIZE];
};

struct transaction_pool tp[SO_TP_SIZE]; /* transaction pool */
int i=0;
int contatore = -1; /* contatore rispetto al primo, mi serve per processare blocchi in 
		   nel registro principale */
		   
int flag=-1;


pid_t vet_nodes[SO_NODES_NUM];
pid_t vet_users[SO_USERS_NUM];
struct dati *d;
int sm;
int semid;
int balance; /* balance del nodo corrente */

FILE* stream;
int qm;

/* SO_BLOCK_SIZE - 1 transazioni possibili da processare, l' ultima e' una transazione
 di pagamento per il processing */

/* se la transaction pool e' piena allora qualsiasi altra transazione verra' scartata
 deve essere informato chi ha inviato la transazione */ 

void _stampa(char * who, char * stampa){ printf("(%s %d) %s \n", who,getpid(), stampa); }
void _error(char * who, char * stampa){ printf("(%s %d) %s \n", who, getpid(), stampa); }
void _scrivo_trans_pool();
int _controllo();
int _scrittura_libro(); 
int _eliminazione_trans_eseguite(); /* dalla transaction pool */
int _controllo_indici();
int _crea_blocco(); /* estrazione di un insieme di SO_BLOCK_SIZE -1 transazioni non ancora presenti nel libro mastro + aggiunta nodo reward (caratteristiche sul pdf) */ 
/* per simulare l' elaborazione, verra' generata un attesa con i valori del pdf */
int _init(void); 
void _init_tp();


int get_random(int num, int plus) {
	
	srand(time(NULL) ^ (getpid()<<16));
	/*printf("rand: %d \n", rand()%num); */
	return (rand() % num)+plus;
}

long get_random2(long min, long max) {
			long tmp;
			
			struct timeval tm;
			gettimeofday(&tm, NULL);
			srandom(tm.tv_sec+tm.tv_usec*1000000ul);
			tmp = random();
			return (tmp % (max+1-min))+min;
}

/* Initialize semaphore to 1 (i.e., "available") */
int initSemAvailable(int semId, int semNum) {
    union semun arg;

    arg.val = 1;
    return semctl(semId, semNum, SETVAL, arg);
}

/* Initialize semaphore to 0 (i.e., "in use") */
int initSemInUse(int semId, int semNum) {
    union semun arg;

    arg.val = 0;
    return semctl(semId, semNum, SETVAL, arg);
}

/* Reserve semaphore - decrement it by 1 */
int reserveSem(int semId, int semNum) {
    struct sembuf sops;

    sops.sem_num = semNum;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    
    printf("(Node %d)valore prima di reserve: %d \n",getpid(), semctl(semid, 0,GETVAL));

    return semop(semId, &sops, 1);
}

/* Release semaphore - increment it by 1 */
int releaseSem(int semId, int semNum) {
    struct sembuf sops;

    sops.sem_num = semNum;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    
    printf("(Node %d)valore prima di release: %d \n", getpid(),semctl(semid, 0,GETVAL));

    return semop(semId, &sops, 1);
}

void _handler_stop(){
	_stampa(USER, "terminazione del nodo corrente");
	exit(0);
}

int main(int argc, char* argv[]){
	
	int f;
	
	printf(" ");
	stream = fopen("log_node.txt", "a");
	f = fileno(stream);
	dup2(f, fileno(stdout));
	fclose(stream);

	stream = fopen("log_node_err.txt", "a");
	f = fileno(stream);
	dup2(f, fileno(stderr));
	fclose(stream);
	
	sm = atoi(argv[0]);
	semid = atoi(argv[1]);
	qm = atoi(argv[2]);
	
	_init_tp(); /* inizializzazione della transaction pool */
	
		
	_stampa(USER, "processo nodo creato");	


	if(signal(SIGTSTP, _handler_stop) == SIG_ERR) {
 				_error(USER,"errore nella gestione dell'handler SIGTSTP");	
 	}
 	else{
 				_stampa(USER, "handler gestito SIGTSTP");
 	}
	
	for(;;) {
		printf("|balance: %d \n", balance);
		_scrivo_trans_pool();  
		_crea_blocco();
	}

	return 0;
}




int _crea_blocco(){ /* appena un blocco della transaction pool è pieno, allora lo inseriamo
		    nel registro mastro */
	
	int k = 0, j = 0, l = 0;
	int sum = 0, var = 0;
	int conto_reward = 0;
	struct transazione tmp; 
	struct transazione blocco[SO_BLOCK_SIZE];
	struct transazione ultima;
	struct timespec tv, tl;
	
	if(flag != -1) {	
		reserveSem(semid, 0);	
				_stampa(USER, "inizio sezione critica init");
				d = (struct dati *)shmat(sm, NULL, 0);	
				printf("|contatore : %d \n", contatore);		
					
				for(k=0; k < SO_BLOCK_SIZE-1; k++) {
					d->lb[d->i].trans[k] = tp[contatore].transazioni[k];
					/*tp[contatore].transazioni[k] = NULL;*/
					conto_reward += tp[contatore].transazioni[k].reward;
					printf("|candidata_sender: %d \n", d->lb[d->i].trans[k].quantita);	
				}
				
				/* ultima transazione candidata del blocco */
				timespec_get(&tv, TIME_UTC);
				ultima.tp = &tv;
				ultima.nsec = tv.tv_nsec;
				ultima.sec = (long)tv.tv_sec;
				ultima.sender = -1;
				ultima.receiver = getpid();
				ultima.quantita = conto_reward;
				ultima.reward = 0;
				
				if(contatore <= d->i){
				d->lb[d->i].trans[k] = ultima;
				}
				
				if(d->i == SO_REGISTRY_SIZE) {
					_stampa(USER, "libro mastro pieno..");
					/* gestione della notifica al master(in caso
					non lo abbia scoperto da solo) */
				}
				else {	
					d->i++;	
				}
				shmdt(d);
				releaseSem(semid, 0);	
				_stampa(USER, "fine sezione critica init");
				flag = -1;
				
				var =get_random2(SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC);
				tl.tv_sec = var/100000000;
				tl.tv_nsec = var % 100000000;
		
		nanosleep(&tl, NULL);
	}
	else { _stampa(USER, "blocco non ancora completo"); }
	
	/* simula l'attesa non attiva */
	/*sleep((get_random(SO_MAX_TRANS_GEN_NSEC, SO_MIN_TRANS_GEN_NSEC))/1000000); */
}

void _init_tp() {

	int k;

	for(k=0; k < SO_TP_SIZE; k++) {
		tp[k].i=0;
		 /* printf("|tp.i = %d \n", tp[i].i); */ 
	}
	
	reserveSem(semid, 0);
	
	d = (struct dati *)shmat(sm, NULL, 0);
	
	for(k=0; k < SO_USERS_NUM; k++) {
		vet_users[k] = d->vet_users[k];
		/* printf("|users %d \n", d->vet_users[k]); */
	}

	for(k=0; k < SO_NODES_NUM; k++) {
		vet_nodes[k] = d->vet_nodes[k];
		/* printf("|nodes %d \n", d->vet_nodes[k]); */
	}

	shmdt(d);
	releaseSem(semid, 0);

}

/*
int _controllo_indici() {
	int rest = 1;
	
	reserveSem(semid, 0);
	_stampa(USER, "inizio sezione critica");

	d = (struct dati *)shmat(sm, NULL, 0);
	printf("|d->re->i: %d\n", d->re->i);
	if(d->re->i == SO_REGISTRY_SIZE) {
		if(d->re->lb[d->re->i]->i == SO_BLOCK_SIZE -1) { rest = 0; }
		else { ++d->re->lb[d->re->i]->i; } 		
	}
	else {
		printf("?\n");
		++d->re->i;
	}
	
	shmdt(d);
	releaseSem(semid, 0);
	_stampa(USER, "fine sezione critica ");
	printf("|rest: %d\n", rest);
	
	return rest;
}
*/
void _handler_sim_stop() {
	_stampa(USER, "terminazione del processo corrente");
	fclose(stream);
	exit(0);
}


void _scrivo_trans_pool(){
	struct trans t;
	int mq;
	
	if((msgrcv(qm, &t, (sizeof(t)-sizeof(long)), 0, 0) == -1)) {
		_error(USER,"msgrcv non accettata! (1)");
		perror("msgget");
	}
	else {
		char print_t[1024];
		char tmp[16];
		struct transazione candidata;
		char buff[100];
		

		/*
		printf("|t->tp: %ld \n", t.tp->tv_nsec);
		*/
		/*timespec_get(ts->tp, TIME_UTC); 
    	strftime(buff, sizeof buff, "%D %T", gmtime(&t.tp->tv_sec));
    	printf("Current time: %s.%09ld UTC\n", buff, t.tp->tv_nsec);
		printf("| tv_nsec %09ld UTC\n", t.tp->tv_nsec);
		*/

		printf("|differenza: %d \n",t.quantita);

		 balance += t.quantita;  
		candidata.tp = t.tp;
		
		candidata.sec = t.sec;
		candidata.nsec = (long)t.nsec;
		/*
		strftime(buff, sizeof buff, "%D %T", gmtime(&candidata.tp->tv_sec));
		*/
		_stampa(USER, "scrivo_trans_pool");
		strcpy(print_t, "sender: ");
		sprintf(tmp, "%d", t.sender);
		candidata.sender = t.sender;
		strcat(print_t , tmp);
		strcat(print_t, " receiver: ");
		sprintf(tmp, "%d", t.receiver);
		candidata.receiver = t.receiver;
		strcat(print_t, tmp);
		strcat(print_t, " quantita': ");
		sprintf(tmp, "%d", t.reward-t.quantita);
		candidata.quantita = t.reward - t.quantita;
		strcat(print_t, tmp);
		strcat(print_t, " reward: ");
		candidata.reward = t.reward;
		sprintf(tmp, "%d", t.reward);
		/*
		strcat(print_t, " time: ");
		sprintf(tmp, "%s", buff);
		strcat(print_t, tmp);
		*/
		strcat(print_t, tmp);
		/*
		strcat(print_t, " time: ");
		strcat(print_t, buff);
		*/
		strcat(print_t, "\n"); 
		
		 /* candidata.sec = t.sec; */
		
 		_stampa(USER, "comunicazione accettata!");
 		
 		
		_stampa(USER, print_t); 
		
		printf("|sender_candidata: %d\n", candidata.sender);
		_stampa(USER, "attendo un attimo"); 
		printf("|i %d tp.i = %d \n",i,tp[i].i);
		
		
		if(tp[i].i == SO_BLOCK_SIZE-1) {
			if(i < SO_TP_SIZE) { i++; contatore++; flag = 0; tp[i].i=0; }
			else {
				char * a = (char *)malloc(sizeof(char)*32);
			char tmp[16];	
			strcpy(a, "kill -s SIGUSR1 ");
			sprintf(tmp, "%d", t.sender);
			strcat(a, tmp);
			if(system(a) != -1) {
				_stampa(USER, "segnale inviato al processo");
			}
			else {
				_error(USER, "segnale non inviato al processo");
			}
			}
		}
		else { tp[i].transazioni[tp[i].i] = candidata; tp[i].i++; _stampa(USER, "salvata in transaction pool"); } 
		
		/*
		if(_controllo_indici() == 1) {
			_stampa(USER,"controllo indici passato!");
			tp[i].transazioni[tp[i].i] = candidata;  
		}
		else {
			char * a = (char *)malloc(sizeof(char)*32);
			char tmp[16];
			_error(USER, "transaction pool piena");	
			strcpy(a, "kill -s SIGTERM ");
			sprintf(tmp, "%d", t.sender);
			strcat(a, tmp);
			if(system(a) == 0) {
				_stampa(USER, "segnale inviato al processo");
			}
			else {
				_error(USER, "segnale non inviato al processo");
			}
			free(a);
		}*/
	}
	
	printf("-\n");
	return;
	
}

int _controllo(int time){
	int rest = 1;	
	int j, k;
	
	reserveSem(semid, 0);
	d = (struct dati *)shmat(sm, NULL, 0);

	/*
	for(j=0; j < d->re->i && rest; j++){
		
		for(k=0; k < d->re->lb[j]->i && rest ; ++k) {
			if(time == d->re->lb[d->re->i]->trans[k]->tp->tv_nsec) {rest = 0;}
		}
	}
	*/

	shmdt(d);	
	releaseSem(semid, 0);
		
	return rest;
}



