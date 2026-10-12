#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <sys/stat.h>


#define SO_MIN_TRANS_GEN_NSEC 100000000
#define SO_USERS_NUM 20
#define SO_NODES_NUM 10
#define SO_REWARD 1
#define SO_MAX_TRANS_GEN_NSEC 200000000
#define SO_RETRY 10	
#define SO_TP_SIZE 100
#define SO_BLOCK_SIZE 10
#define SO_MIN_TRANS_PROC_NSEC
#define SO_MAX_TRANS_PROC_NSEC
#define SO_REGISTRY_SIZE 1000
#define SO_BUDGET_INIT 1000
#define SO_SIM_SEC 10
#define SO_FREINDS_NUM 10

#define qm_key 1234


struct transazione {
	struct timespec *tp;
	long nsec;
	long sec;
	pid_t sender; 
	pid_t receiver;
	int quantita; 
	int reward;
};

struct trans {
	long mtype;
	struct timespec *tp;
	long nsec;
	long sec;
	pid_t sender; 
	pid_t receiver;
	int quantita; 
	int reward;
};

struct libro_mastro{
	struct transazione trans[SO_BLOCK_SIZE];
};

struct dati {
	pid_t vet_users[SO_USERS_NUM];
	pid_t vet_nodes[SO_NODES_NUM];
	struct libro_mastro lb[SO_REGISTRY_SIZE]; 
	int i; /* contatore del libro mastro */
};


union semun{
	int val;
	struct semid_ds* buf;
	unsigned short * array;
	struct sminfo* __buf;
};

