#include "lib.h"

#define USER "user"

struct dati *d;
pid_t vet_nodes[SO_NODES_NUM];
pid_t vet_users[SO_USERS_NUM];
int sm;
int semid;
int qm;

void __init(void); 
void _stampa(char * who, char * stampa){ printf("(%s %d) %s \n", who,getpid(), stampa); }
void _error(char * who, char * stampa){ printf("(%s %d) %s \n", who, getpid(), stampa); }
void _calcolo_bilancio(); 
int _crea_transazione(); 
void _invio_transazioni(int, int, int, int);
void _handler_transazione();
int bilancio(void);
void _handler_sim_stop();
void _handler_tp_piena();

int saldo; /* budget iniziale */
int balance; /* ci serve per calcolare il bilancio */
int invio_trans; /* ci permette di capire il numero di tentativi effetuati ad inviare la transazione */
FILE* stream;

struct transazione t[SO_BLOCK_SIZE]; /* mi tengo im memoria le transazioni inviate, cosi' posso calcolarmi il bilancio */

int i=0; /* ci serve come indice per identificare le transazioni memorizzate */


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
    
    printf("(User %d)valore prima di reserve: %d \n",getpid(), semctl(semid, 0,GETVAL));

    return semop(semId, &sops, 1);
}

/* Release semaphore - increment it by 1 */
int releaseSem(int semId, int semNum) {
    struct sembuf sops;

    sops.sem_num = semNum;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    
    printf("(User %d)valore prima di release: %d \n", getpid(),semctl(semid, 0,GETVAL));

    return semop(semId, &sops, 1);
}

int main(int argc, char* argv[]){

	struct timespec tp; 
	long var = 0;

	printf(" ");
	/*printf("|pid : %d\n", getpid());*/			
	sm = atoi(argv[0]);
	semid = atoi(argv[1]);
 	qm = atoi(argv[2]);
	 __init();
	
	balance = saldo;
	
	
	for(;;){
		balance = bilancio();
		printf("| balance: %d \n", balance);
		if(balance >= 2) {
			_stampa(USER, "bilancio > 2");
			_calcolo_bilancio();
		}
		/* else if(balance == -1) { _stampa(USER, "lista privata di transazioni piena "); exit(0);} */
		else { _stampa(USER, "bilancio < 2"); /* non faccio niente */ }
		
		var = get_random2(SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC);
		tp.tv_sec = var/100000000;
		tp.tv_nsec = var % 100000000;
		
		nanosleep(&tp, NULL);
	}	


	return 0;
}

void _handler_vuota() { _stampa(USER, "sono vivo"); }

void __init() {	
	int k = 0;
	int f;

	printf(" ");
	
	stream = fopen("log_user.txt", "a");
	f = fileno(stream);
	dup2(f, fileno(stdout));
	fclose(stream);

	stream = fopen("log_user_err.txt", "a");
	f = fileno(stream);
	dup2(f, fileno(stderr));
	fclose(stream);

	
	_stampa(USER, "processo utente creato");


	reserveSem(semid, 0);
	_stampa(USER, "inizio sezione critica");

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
		
	_stampa(USER, "fine sezione critica");

	/* fine sezione critica */

	
	saldo = SO_BUDGET_INIT; /* inizializzazioe buget iniziale */ 
	invio_trans = 0; /* ci permette di calcolare se il processo non riesce per tot volte consecutive a 
			  generare una transazione */
	
	/*
	if(signal(SIGINT, _handler_transazione) == SIG_ERR) {
 				_error(USER,"errore nella gestione dell'handler");	
 	}
 	else{
 				_stampa(USER, "handler SIGINT gestito");
 	}*/
 	
 	if(signal(SIGUSR1, _handler_tp_piena) == SIG_ERR) {
 				_error(USER,"errore nella gestione dell'handler SIGUSR1");	
 	}
 	else{
 				_stampa(USER, "handler SIGUSR1 gestito");
 	}


	 if(signal(SIGPIPE, _handler_vuota) == SIG_ERR) {
 				_error(USER,"errore nella gestione dell'handler");	
 	}
 	else{
 				_stampa(USER, "handler reimpostato");
 	}
 	
 	if(signal(SIGTSTP, _handler_sim_stop) == SIG_ERR) {
 				_error(USER,"errore nella gestione dell'handler SIGTSTP");	
 	}
 	else{
 				_stampa(USER, "handler SIGTSTP gestito");
 	}
}

void _handler_tp_piena() {
 	_stampa(USER, "transaction pool piena");
}

void _handler_sim_stop() {
	_stampa(USER, "terminazione del processo corrente");
	fclose(stream);
	exit(0);
}

int bilancio() {

	int tot = 0, sum = 0, tot2 = 0;
	int k=0, j=0,ret=0;


	reserveSem(semid, 0);	
	_stampa(USER, "inizio sezione critica del bilancio");
	/* printf("dopo \n"); */
	d = (struct dati *)shmat(sm, NULL, 0);
	
	if(d->i > 0) {
	for(k=0; k < d->i; ++k) {
		for(; j < SO_BLOCK_SIZE; j++) {
			if(d->lb[d->i].trans[j].sender == getpid()) {
			tot += d->lb[d->i].trans[j].quantita;
			}
		}
	 }
	 for(k=0; k < d->i; ++k) {
		for(j=0; j < SO_BLOCK_SIZE; j++) {
			if(d->lb[d->i].trans[j].receiver == getpid()) {
			tot2 += d->lb[d->i].trans[j].quantita;
			printf("| transazione k: %d \n", j);
			}
		}
	 }
	 	ret = tot2 - tot;
	}
	else{
	for(; k < i; k++) {
		sum += t[k].quantita; /* mi sommo tutte le transazione da me inviate */
		printf("| transazione k: %d su %d\n", k, i); 
	}
	/*printf("| quantita: %d \n", sum);*/
	
	ret = balance - sum;
	}
	printf("|ret: %d\n", ret);
	shmdt(d);
	
	releaseSem(semid, 0);	
	_stampa(USER, "fine sezione critica del bilancio");
	
	return (ret <= 0)?balance-ret:ret;
}

void _calcolo_bilancio() {
	int rw, user_scelto, sum_nodo, sum_utente;
	int user_estratto = 0;
	
	/* tot = entrate - uscite */ 
	
	/* tot >= 2 -> */ 
	
	user_scelto = get_random(SO_USERS_NUM,0);
	while(vet_users[user_scelto] == getpid()) {
		user_scelto = get_random(SO_USERS_NUM,0);}

	rw = (get_random(balance,2)); /* calcolo del valore estratto: tra 2 e il bilancio calcolato prima */
	
	printf("|rw: %d \n", rw);
	
	sum_nodo = (int)((double)((double)(SO_REWARD/100.0)*rw)); /* invio questo denaro al nodo x */
	printf("|sum_nodo: %d \n", sum_nodo);

	sum_utente = rw - sum_nodo; /* invio questo denaro all' utente x */
	
	user_estratto = vet_users[user_scelto];
	_invio_transazioni(sum_nodo, rw, user_estratto, 1);
	return;
}

void _handler_transazione(){

	int re = get_random(SO_USERS_NUM,1); /* estraggo un node da 1 a SO_USERS_NUM  */ 	

	/*
	if(signal(SIGINT, _handler_transazione) == SIG_ERR) {
 				_error(USER,"errore nella gestione dell'handler");	
 	}
 	else{
 				_stampa(USER, "handler reimpostato");
 	}
 	*/
 	
 	/* genero la transazione */
 	
 	_stampa(USER, "generazione della transazione dopo il segnale SIGINT(esterno)");
	_invio_transazioni(0, 0, vet_users[re], 0);
	
	return;

	/* gestione della SIGINT (kill del processo) */
	
	/* deve inviare una transazione in risposta al segnale */
	
	
	/* se non riesce entro SO_RETRY volte allora termina l' esecuzione
	 notificando il master */
}

void _invio_transazioni(int soldi, int reward, int r, int flg){
		ssize_t inv;
		struct trans tr; struct transazione ins; struct timespec tv;	
		char print_t[512];
		char tmp[16];
		
		printf("soldi: %d \n", soldi);
		
		/*clock_gettime(CLOCK_MONOTONIC, &tv);*/
		timespec_get(&tv, TIME_UTC);
		ins.nsec = tr.nsec = tv.tv_nsec; 
		ins.sec = tr.sec = (long)tv.tv_sec; 
		ins.tp = tr.tp = &tv;
		ins.quantita = tr.quantita = soldi;
		ins.reward = tr.reward = reward;
		ins.receiver = tr.receiver = r;
		ins.sender = tr.sender = getpid();
		tr.mtype = 1;

		/*
		if(flg == 1) {
		tr.mtype = vet_nodes[get_random(SO_NODES_NUM, 0)];
		}
		else { tr.mtype = 1; }		
		*/
		/*clock_gettime(clock_getcpuclockid, t->tp);*/ 
		
		
		/* -> vet_nodes[r] accesso al pid del nodo a cui voglio mandare la transazione */
		
		if(i <= SO_BLOCK_SIZE) {

		t[i] = ins;
		i++;
		
		if((inv = msgsnd(qm, &tr, (sizeof(tr)-sizeof(tr.mtype)),0)) == -1){
			_error(USER, "errore invio transazione");
			perror("msgget");
			invio_trans++; 
			if(invio_trans == SO_RETRY) { 
				/* notifico il master */
				_stampa(USER, "ho provato a inviare il messaggio ma non ci sono riuscito");
				exit(0); /* esco */
			 }
		}	
		else{
		char print_t[512];
		char tmp[16];
		char buff[100];
		char mtype[32];
		
		invio_trans = 0; /* resetto il contatore */
		snprintf(mtype, sizeof(mtype),"%lu", tr.mtype);
		
			/*timespec_get(t->tp, TIME_UTC);*/
    	strftime(buff, sizeof(buff), "%D %T", gmtime((time_t *)&ins.sec));
    	/*
    	printf("Current time: %s.%09ld UTC\n", buff, ins.tp->tv_nsec);
	*/
		/*
		printf("| tv_nsec %09ld UTC\n", ins.tp->tv_nsec);
		*/
		
		strcpy(print_t, "sender: ");
		sprintf(tmp, "%d", tr.sender);
		strcat(print_t , tmp);
		strcat(print_t, " receiver: ");
		sprintf(tmp, "%d", tr.receiver);
		strcat(print_t, tmp);
		strcat(print_t, " quantita': ");
		sprintf(tmp, "%d", tr.reward-tr.quantita);
		strcat(print_t, tmp);
		strcat(print_t, " reward: ");
		sprintf(tmp, "%d", tr.reward);
		strcat(print_t, tmp);
		
		strcat(print_t, " time: ");
		strcat(print_t, buff);
		strcat(print_t, " mtype: ");
		strcat(print_t, mtype);
		strcat(print_t, "\n"); 
		_stampa(USER, print_t); 
			/*free(t); */

			_stampa(USER, "transazione inviata"); 
		}
		
	
		_stampa(USER,"attendo un attimo");
		}
		else {
			_stampa(USER, "resetto");
		}
		/*sleep((get_random(SO_MAX_TRANS_GEN_NSEC,SO_MIN_TRANS_GEN_NSEC))/1000000); */
		return;
		/* + attesa di random(so_min_trans_gen_nsec e so_max_trans_gen_nsec) <+>*/
}

