
#include "lib.h"
#define USER "master"


/* shared memories */

struct dati * d; /* struttura vera e propria */ 
int sm; /* shared memory id*/
int semid;
int qm;
pid_t utenti[SO_USERS_NUM];
pid_t nodi[SO_NODES_NUM];
pid_t budget_maggiore[SO_USERS_NUM];

/* end shared memories */

void _stampa(char * who, char * stampa){ printf("(%s %d) %s \n", who,getpid(), stampa); }
void _error(char * who, char * stampa){ printf("(%s %d) %s \n", who, getpid(), stampa); }
void __init(void);
void _stampa_repilogo(int);
void _deallocazione(void);
void _init_user(void);
void _handler_stop();
void _init_reg();

int get_random(int num, int plus) {
	
	srand(time(NULL) ^ (getpid()<<16));
	/*printf("rand: %d \n", rand()%num); */
	return (rand() % num)+plus;
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


void stampa_continua(){
	int j, l, m;
	int tot_entrate = 0, tot_uscite = 0;
	char buff[64];
	
	d = (struct dati *)shmat(sm, NULL, 0);

	printf("USERS attivi e non: \n");
	if(SO_USERS_NUM <= 100) {

	for(j=0; j < SO_USERS_NUM; j++) {
			for(l=0; l < d->i; l++) {
				for(m=0; m < SO_BLOCK_SIZE; m++) {
			if(d->vet_users[j] == d->lb[l].trans[m].sender) { tot_uscite += d->lb[l].trans[m].quantita; }
			else if(d->vet_users[j] == d->lb[l].trans[m].receiver) { tot_entrate += d->lb[l].trans[m].quantita; }
		}
	}
			snprintf(buff, sizeof(buff), "kill -s PIPE %d", d->vet_users[j]);

			if(system(buff) != -1) { printf("| processo %d attivo, budget = %d \n", d->vet_users[j], tot_entrate - tot_uscite);  }
			else { printf("| processo %d non attivo, budget = %d \n", d->vet_users[j], tot_entrate - tot_uscite);  }
	}

	}
	else {
		for(j=0; j < SO_USERS_NUM; j++) {
			for(l=0; l < d->i; l++) {
				for(m=0; m < SO_BLOCK_SIZE; m++) {
			if(d->vet_users[j] == d->lb[l].trans[m].sender) { tot_uscite += d->lb[l].trans[m].quantita; }
			else if(d->vet_users[j] == d->lb[l].trans[m].receiver) { tot_entrate += d->lb[l].trans[m].quantita; }
			}
		}
			if((tot_entrate - tot_uscite) > SO_BUDGET_INIT) {
			snprintf(buff, sizeof(buff), "kill -s PIPE %d", d->vet_users[j]);

			if(system(buff) != -1) { printf("| processo %d attivo, budget = %d \n", d->vet_users[j], tot_entrate - tot_uscite);  }
			else { printf("| processo %d non attivo, budget = %d \n", d->vet_users[j], tot_entrate - tot_uscite);  }
			}
	}
}
	
	for(j=0; j < d->i; j++) {
		printf("registro (%d / %d) \n",j+1, SO_REGISTRY_SIZE);
		printf("\n\t");
		for(l=0; l < SO_BLOCK_SIZE; l++) {
			strftime(buff, sizeof buff, "%D %T", gmtime((time_t *)&d->lb[j].trans[l].sec));
			printf("blocco (%d / %d)", l+1, SO_BLOCK_SIZE);
			printf("\n\t\tsender: %d receiver: %d quantita: %d reward: %d time: %s",d->lb[j].trans[l].sender,d->lb[j].trans[l].receiver,d->lb[j].trans[l].quantita, d->lb[j].trans[l].reward, buff);
			printf("\n\t");
		}
	}
	
	
	shmdt(d);
}


/* creazione di una coda di messaggi da condividere tra nodi e utenti */

int main(int argc, char *argv[]){
	int k = 0;
	int j,l,m;
	char buff[100];
	/*char buff[100];*/
	_init_reg(); /* inizializzazione di tutti i tipi derivati, struct, ecc. */ 
	__init(); /* genero tutti gli utenti e gestisco la terminazione della simulazione in SO_SIM_SEC  */


	for(;k < SO_SIM_SEC; k++){
	
		
		reserveSem(semid, 0);
		stampa_continua();
		releaseSem(semid, 0);
		printf("\n");	
		/* sleep((get_random(SO_MAX_TRANS_GEN_NSEC,SO_MIN_TRANS_GEN_NSEC))/1000000); */
		 sleep(5); 
	} /* ciclio di vita del master*/ 
		
	_stampa(USER, "intermezzo stampa");	
	_handler_stop(); 
	return 0;
}

void _init_reg() {

	int i=0, k=0;
	
	/* dichiarazione della shared memory */
	
	 /* d = (struct dati *)malloc(sizeof(struct dati)); */

		if((sm = shmget(IPC_PRIVATE, sizeof(struct dati), IPC_CREAT | 0666)) == -1) {
			_error(USER,"errore msg queue non creata!");
		}
		else {
			_stampa(USER,"shared memory creata!");
		}
		
	 /* d = (struct dati *)malloc(sizeof(struct dati)); */
	d = (struct dati *)shmat(sm, NULL, 0);
	shmdt(d);
}

void __init() {
		char tmp1[20];
		char tmp2[20];
		char tmp3[20];
		int i=0, f=0; 
		pid_t child = 0;
		FILE *stream;
		FILE * stream2; 
		struct msqid_ds tmp;

		
		if(signal(SIGINT, _handler_stop) == SIG_ERR) {
 				_error(USER,"errore nella gestione dell'handler");	
 		}
 		else{
 				_stampa(USER, "handler gestito");
 		}

		  /* creazione semaforo */

		 if((semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666) )== -1){
    			_stampa(USER, "errore creazione semaforo");
 		 }
 		 else { _stampa(USER, "semaforo creato"); }
  		/* semaforo inizializzato a 1*/
  		
  		if(initSemAvailable(semid,0)== -1){
    			_stampa(USER, "errore inizializzazione semaforo");
  		}
		else { _stampa(USER, "semaforo inializzato"); }
	
		/* per deallocarli free(vet_users) && free(vet_nodes) */
		
		/* passi tot -> la simulazione termina || la capacita' del libro mastro esaurisce */

	


		if( (qm = msgget(qm_key, IPC_CREAT | 0666 )) == -1){ /*-> creazione della coda (verra' ereditata da tutti con la stessa chiave) */
				_error(USER,"errore msg queue non creata!");
				perror("msgget");
		}
		else {
			_stampa(USER,"msg queue creata!");
		}
	
		printf("master qm = %d\n", qm);

		/* reg = (struct registro *) shmat(sm, NULL, 0); */
		
		/* shmdt(r); */

		/* fine dichiarazione della shared memory */

		/*
		sprintf(p2, "%d", qm);

		in = fopen("pid_user", "w");
		*/
		/*
		fclose(in);
		
		*/
			
		
		/*	
		in = fopen("pid_node", "w");
		*/
		/*
		fclose(in);		
		*/

		snprintf(tmp1, sizeof(tmp1),"%d", sm);
		snprintf(tmp2, sizeof(tmp2),"%d", semid);
		snprintf(tmp3, sizeof(tmp3),"%d", qm);
		
		printf("|tmp3 %s \n",tmp3);
	
		stream = fopen("log_user.txt", "a");
		fclose(stream);	
		stream2 = fopen("log_node.txt", "a"); 
		fclose(stream2);

		stream = fopen("log_user_err.txt", "a");
		fclose(stream);	
		stream2 = fopen("log_node_err.txt", "a"); 
		fclose(stream2);

		/*if(stream == NULL) { _stampa(USER, "stream null"); } */


		_stampa(USER, "inizio sezione critica init");
		reserveSem(semid, 0);	
	
		d = (struct dati *)shmat(sm, NULL, 0);
	
		
		for(i=0; i < SO_NODES_NUM; i++) {
		      if((child = fork() )== 0) {		
			if(execl("./nodo", tmp1, tmp2, tmp3,NULL) != -1){
				break;
			}
			else {	
				exit(0);
			}

		  	}
			else { /*printf("t %d %d\n", i,child);*/ d->vet_nodes[i] = child; }
			
		}

		_stampa(USER, "intermezzo sezione critica");

		for(i=0; i < SO_USERS_NUM; i++) {
			if((child = fork() )== 0) { 
			if(execl("./utente", tmp1, tmp2, tmp3,NULL) != -1){
				break;
			}
			else {
				exit(0);
			}
		       }
			else {  d->vet_users[i] = child; /* printf("%d %d \n", i,d->vet_users[i]);*/ } 
			
		}
		
		shmdt(d);
		releaseSem(semid, 0);	
		_stampa(USER, "fine sezione critica init");
}

void _handler_stop() {
	int i=0;
	
	if(signal(SIGTSTP, _handler_stop) == SIG_ERR) {
 				_error(USER,"errore nella gestione dell'handler");	
 	}
 	else{
 				_stampa(USER, "handler reimpostato");
 	}
 	
 
	_stampa(USER, "sto per terminare tutta la simulazione");
	_stampa(USER, "inizio regione critica");
	reserveSem(semid, 0);	
	d = (struct dati *)shmat(sm, NULL, 0); 

 	for(i=0; i < SO_USERS_NUM; ++i) { 		
		if(kill(d->vet_users[i], SIGTSTP) != -1) {_stampa(USER, "processo utente ucciso");} /* la SIGSTP */
 	}
 	
 	for(i=0; i < SO_NODES_NUM; ++i) {
		if(kill(d->vet_nodes[i], SIGTSTP) != -1) {_stampa(USER, "processo nodo ucciso");}
 	}
	
	_stampa(USER, "fine regione critica");
	shmdt(d);
	releaseSem(semid, 0);

	if(msgctl(qm, IPC_RMID, NULL) == -1) {
		_stampa(USER, "errore eliminazione qm");
	}
	else  { _stampa(USER, "qm eliminata");  }

	if(semctl(semid,0, IPC_RMID) == -1) {
		_stampa(USER, "errore eliminazione semaforo");
	}
	else  { _stampa(USER, "semaforo eliminato");  }
	
	if(shmctl(sm, IPC_RMID, NULL) == -1) {
		_stampa(USER, "errore eliminazione sm");
	}
	else  { _stampa(USER, "sm eliminata");  }
	
	exit(0);
 	
	
	/*_stampa_repilogo(0);*/
	return;
}

/* 0 -> sono terminati i tot secondi di simulazione */
/* 1 -> ctrl + c */
/* 2 -> tutti gli utenti sono terminati */

void _stampa_repilogo(int flag){

	if(flag == 0) {
		int k = 0; int j=0;
		_stampa(USER, "simulazione terminata");

		
		reserveSem(semid, 0);	
		d = (struct dati *)shmat(sm, NULL, 0);
			
			/*
		for(; k < d->i; k++) {
			printf("%d registro: \n", k+1); 
			for(; j < d->i; j++) {
				printf("transazione: %d\t sender: %d receiver: %d quantita': %d\n", j+1, d->re->lb[k]->trans[j]->sender, d->re->lb[k]->trans[j]->receiver, d->re->lb[k]->trans[j]->quantita);
			}				
		}
			*/
		shmdt(d);

		releaseSem(semid, 0);		
	}
	

	if(msgctl(qm, IPC_RMID, NULL) == -1) {
		_stampa(USER, "errore eliminazione qm");
	}
	else  { _stampa(USER, "qm eliminata");  }

	if(semctl(semid,0, IPC_RMID) == -1) {
		_stampa(USER, "errore eliminazione semaforo");
	}
	else  { _stampa(USER, "semaforo eliminato");  }
	
	if(shmctl(sm, IPC_RMID, NULL) == -1) {
		_stampa(USER, "errore eliminazione sm");
	}
	else  { _stampa(USER, "sm eliminata");  }
	
	exit(0);
}
