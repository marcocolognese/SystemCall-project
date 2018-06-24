/**
 * @file father.c
 * @author Marco Colognese
*/

#include "mylib.h"
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>

/**
 * @brief Programma in C che utilizzi le system call (IPC), ove possibile, per implementare un simulatore di calcolo parallelo.
 *
 *	Il programma dovrà leggere un file di configurazione, contenente:
 *		1. Il numero di processi di calcolo parallelo.
 *		2. I dati di computazione.
 *	
 *	Il file di configurazione avra' la seguente struttura:
 *		1. La prima riga conterra' un intero, che corrisponde al numero di processi paralleli da creare, NUM_PROC.
 *		2. Le altre righe avranno il seguente formato: <id> <num1> <op> <num2>, dove:
 *			1. id e' il numero di processo parallelo a cui far eseguire l'operazione, se e' maggiore o
 *			   uguale a 1. Se e' zero, indica che l'operazione dovra' essere eseguita dal primo processo
 *			   libero. Si assuma che id corrisponda sempre ad un valore corretto.
 *			2. num1 e num2 sono due numeri interi, corrispondenti ai dato sui quali eseguire l'operazione.
 *			3. op e' un carattere rappresentante l'operazione da eseguire sui due numeri. Caratteri validi per op sono:
 *			   '+', '-', '*', '/', rappresentanti rispettivamente la somma, la sottrazione, la moltiplicazione e la divisione.
 *
 *	Ogni processo eseguirà una routine di questo tipo:
 *		1. Attesa su un semaforo per eseguire un calcolo.
 *		2. Ricezione dei dati e dell'operazione da eseguire, da parte del processo padre.
 *			1. Se riceve il comando di terminazione 'K', termina la sua esecuzione.
 *		3. Esecuzione del calcolo.
 *		4. Invio del risultato al padre, e segnalazione al padre che ha finito di calcolare.
 *		5. Attesa bloccante sullo stesso semaforo al punto 1, per attendere il comando successivo.
 *
 *	Il processo padre comunichera' con i processi figlio tramite memoria condivisa.
 *
 *	Il processo padre eseguira' in questo modo:
 *		1. Setup della simulazione, leggendo dal file di configurazione il numero di processori da
 *	   	   simulare, creandone i processi relativi, e creando ed inizializzando le eventuali strutture di
 *		   supporto quali semafori e memoria condivisa. Inoltre, verra' creato un array in cui
 *		   memorizzare i risultati delle operazioni eseguite. La lunghezza di tale array sara' ricavata dal
 *		   numero di operazioni lette dal file di configurazione.
 *		2. Entrata in un ciclo che per ogni operazione da simulare effettua quanto segue:
 *			1. Se il comando ha id diverso da zero, attende che il processo numero id sia libero, salva il
 *			   risultato dell'eventuale calcolo precedente nell'array dei risultati, e poi interagisce con lui
 *			   passandogli il comando da simulare. Il processo padre non deve attendere che il processore
 *			   abbia completato la simulazione dell'operazione passata al figlio.
 *			2. Se l'istruzione ha id 0, trova il primo processore libero ed in caso interagisce con esso,
 *			   come al punto 1. Altrimenti attende che almeno un processore sia libero, e poi interagisce
 *			   con esso come al punto 1.
 *		3. Passati tutti i comandi ai figli, attende che i processi figlio abbiano eseguito tutti i loro calcoli.
 *		4. Salva nell'array dei risultati gli ultimi risultati computati, e fa terminare i processi figlio
 *		   passando il comando di terminazione 'K'.
 *		5. Attende che tutti i figli abbiano terminato.
 *		6. Stampa su un file di output tutti i risultati.
 *		7. Libera eventuali risorse.
 *		8. Esce.
 *
*/

void main (void){

	int fd, r;		//file descriptor e lettura del file
	int j=0, i=0;		//contatori
	int righe=-2;		//escludo il primo e l'ultimo \n
	int NUM_PROC=0;		//numero di processi da creare
	const char *file_name="file.txt";
	char buffer[BUFLEN];	//buffer di lettura
	char buf;		//carattere di lettura
	char stampa[256];	//array di char per le stampe di sprintf



	//APERTURA DEL FILE
	fd=open(file_name, O_RDONLY);
	if(fd==-1){
		write(STDOUT, "Errore in apertura del file\n", strlen("Errore in apertura del file\n"));
		exit(1);
	}
	
	
	// LEGGO IL NUMERO DI OPERAZIONI DA SVOLGERE
	while(r!=0){
		r=read(fd, &buf, 1);
		if(r==-1){
			write(STDOUT, "Errore in lettura del file\n", strlen("Errore in lettura del file\n"));
			exit(1);
		}
		if(buf=='\n')
			righe++;
	}
	// riposiziono il cursore ad inizio file
	lseek(fd, 0, 0);
	
	
//LETTURA DEL NUMERO DI PROCESSI DA CREARE
	do{
		r=read(fd, &buf, 1);
		buffer[i]=buf;
		if(r==-1){
			write(STDOUT, "Errore in lettura del file\n", strlen("Errore in lettura del file\n"));
			exit(1);
		}
		if(r==0){
			write(STDOUT, "File vuoto\n", strlen("File vuoto\n"));
			exit(1);
		}
		i++;
	}
	while(buf!='\n');
	NUM_PROC=calcolo_char(buffer, i-1);

	if (NUM_PROC < 1){
		write(STDOUT, "Numero processi inferiore a 1\n", strlen("Numero processi inferiore a 1\n"));
		exit(1);
	}
	
//###############################################################################################//
//						MEMORIA CONDIVISA	    			 //
//###############################################################################################//
///
///Breve schema riguardante lo svolgimento del programma
///
///1- Creazione della memoria condivisa
	share_mem * memoria[NUM_PROC];
	int shm_id[NUM_PROC];
	int memkey;
	
	for(j=0; j<NUM_PROC; j++){
		memkey = ftok("father.c", j);
		if((shm_id[j] = shmget(memkey, sizeof(share_mem), IPC_CREAT|0666))==-1){
			write(STDOUT, "Allocazione memoria condivisa fallita\n", strlen("Allocazione memoria condivisa fallita\n"));
        		exit(1);
    		}
    		//attacco la memoria alla zona dati del padre (verrà ereditata anche dai figli una volta creati)
    		if((memoria[j] = shmat(shm_id[j], NULL, 0))==(share_mem *)-1){
    			write(STDOUT, "Memoria non attaccata\n", strlen("Memoria non attaccata\n"));
        		exit(1);
        	}
    	}
    	
    	write(STDOUT, "\nMemoria condivisa allocata e attaccata correttamente\n\n", strlen("\nMemoria condivisa allocata e attaccata correttamente\n\n"));
	
//###############################################################################################//
//					SEMAFORI						 //
//###############################################################################################//
///2- Creazione dell'array dei semafori
    	// creo lo spazio per 2 operazioni sull'array di semafori
    	struct sembuf * sops = (struct sembuf *) malloc (2*sizeof(struct sembuf));
    
	int semaforo;
	int semkey;

	semkey = ftok("father.c", j);
	
	// Semafori dispari=padre	Semafori pari=figli
	if((semaforo = semget(semkey, (2*NUM_PROC)+1, IPC_CREAT | IPC_EXCL | 0666))==-1){
		write(STDOUT, "Creazione semaforo non riuscita\n", strlen("Creazione semaforo non riuscita\n"));
       		exit(1);
	}
	
	//inizializzo a 1 i semafori dei figli che attenderanno di essere sbloccati dal padre quando passerà loro i calcoli da svolgere
	for(j=0; j<NUM_PROC; j++){
		sops[0].sem_num = 2*j;
    		sops[0].sem_op = 1;
    		sops[0].sem_flg = 0;
    		semop(semaforo, sops, 1);
	}
	
	//semaforo intero inizializzato a NUM_PROC per controllare se ci sono figli liberi (inizialmente tutti liberi)
	sops[0].sem_num = 2*NUM_PROC;
    	sops[0].sem_op = NUM_PROC;
    	sops[0].sem_flg = 0;
    	semop(semaforo, sops, 1);
    	
    	write(STDOUT, "Semafori creati correttamente\n\n", strlen("Semafori creati correttamente\n\n"));
    	
    		
	
//###############################################################################################//
//					CREAZIONE DEI PROCESSI FIGLI				 //
//###############################################################################################//
///3- Creazione dei processi figli
	
	int id=0;
	pid_t processi[NUM_PROC];
	//solo il padre deve continuare a rimanere nel ciclo (i figli non devono fare fork());
	for(i=0; i<NUM_PROC; i++){
		processi[i]=fork();
		if(processi[i]<0){		//fork() fallita
			write(STDOUT, "Fork fallita\n", strlen("Fork fallita\n"));
			exit(1);
		}
		//assegno un ID=i+1 a ciascun figlio i. Il padre sarà l'unico ad avere ID=0. Il figlio poi esce dal ciclo.
		else if(processi[i]==0){
			id=i+1;
			break;
		}
		else{		//codice del padre
			sprintf(stampa, "\tPADRE: figlio %d creato correttamente\n", i+1);
			write(STDOUT, stampa, strlen(stampa));
		}
	}	//fine ciclo for, figli creati correttamente
	
	int val, op1, op2;	//processo che svolgerà l'operazione e operandi
	char oper1[BUFLEN];	//buffer per la lettura del primo operando
	char oper2[BUFLEN];	//buffer per la lettura del secondo operando
	char res[BUFLEN];	//buffer per la lettura del risultato dell'operazione
	char calc;		//operazione da svolgere
	int counter=0;		//contatore per l'array dei risultati
	char *risultati[righe];	//array dei risultati
	
	//allocazione della memoria per l'array dei risultati
	for(j=0; j<righe; j++)
		risultati[j]=(char *)malloc(32*sizeof(char));
		
		
	
//###############################################################################################//
//					PADRE (ID=0)						 //
//###############################################################################################//
///4- Assegnazione ai figli delle operazioni da svolgere
	
	if(id==0){
		int space=0;	//contatore per gli spazi tra campi dell'operazione da svolgere
		do{
			i=0;	//contatore per il buffer di lettura
			space=0;
			do{
				r=read(fd, &buf, 1);
				if(r==-1){
					write(STDOUT, "Errore in lettura del file\n", strlen("Errore in lettura del file\n"));
					exit(1);
				}

				//controllo se sono alla fine del file (doppio \n)
				if(space==0 && buf=='\n')
					break;
				
				if(buf=='\n'){
					op2=calcolo_char(buffer, i);
					i=0;
				}

				// verifico se c'è uno spazio
				if(buf!=' '){
					buffer[i]=buf;
					i++;
				}
				else{
					switch(space){
						case 0:	//al primo spazio sto leggendo il numero del processo
							val=calcolo_char(buffer, i);
							break;
						case 1:	//al secondo spazio sto leggendo il primo operando
							op1=calcolo_char(buffer, i);
							break;
						case 2:	//al terzo spazio sto leggendo il calcolo da eseguire
							calc=buffer[0];
							break;			
						default://al quarto spazio sto leggendo il secondo operando (già letto sopra)
							break;
					}
					i=0;
					space++;
				}
			} while(buf!='\n' && r>0);
			
			//se sono alla fine del file (doppio \n) interrompo il ciclo
			if(space==0 && buf=='\n')
				break;
			
			//verifico che ci siano figli liberi (decremento semaforo intero)
			sops[0].sem_num = 2*NUM_PROC;
    			sops[0].sem_op = -1;
    			sops[0].sem_flg = 0;
    			semop(semaforo, sops, 1);
    			
    			//ricerca del primo processo libero se VAL=0
    			if (val==0){	//verifico il primo processo libero guardando i valori dei semafori (con il flag IPC_NOWAIT)
    				for(j=0; j<NUM_PROC; j++){
	    				sops[0].sem_num = (2*j)+1;
	    				sops[0].sem_op = 0;
	    				sops[0].sem_flg = IPC_NOWAIT;
	    				if (semop(semaforo, sops, 1) != -1)
	    					break;
	    			}
	    			val=j+1;
	    			
	    			sprintf(stampa, "\tPADRE: ho cercato un processo libero. Ho trovato %d\n", val);
				write(STDOUT, stampa, strlen(stampa));
    			}	
			
			sprintf(stampa, "\tPADRE: assegno il calcolo %d%c%d al figlio %d\n", op1, calc, op2, val);
			write(STDOUT, stampa, strlen(stampa));
			
			//attendo che il figlio abbia finito di calcolare eventuali operazioni precedenti
			sops[0].sem_num = 2*(val-1)+1;
			sops[0].sem_op = 0;
			sops[0].sem_flg = 0;
			sops[1].sem_num = 2*(val-1)+1;
			sops[1].sem_op = 1;  		// mette a 1 il suo semaforo così non rientra subito
			sops[1].sem_flg = 0;
			semop(semaforo, sops, 2);
	
			
			//se il figlio ha già fatto dei calcoli, prelevo il risultato
			if(memoria[val-1]->finish==true){
				itoa(memoria[val-1]->res, res);
				itoa(memoria[val-1]->val1, oper1);
				itoa(memoria[val-1]->val2, oper2);
	
				strcat(risultati[counter], oper1);
				strcat(risultati[counter], &(memoria[val-1]->op));
				strcat(risultati[counter], oper2);
				strcat(risultati[counter], "=");
				strcat(risultati[counter], res);
				strcat(risultati[counter], "\n");
				counter++;
				memoria[val-1]->finish=false;
				
				sprintf(stampa, "\tPADRE: ho prelevato il risultato: %d%c%d=%d del figlio %d\n", memoria[val-1]->val1, memoria[val-1]->op, memoria[val-1]->val2, memoria[val-1]->res, val);
				write(STDOUT, stampa, strlen(stampa));
				sprintf(stampa, "\tPADRE: assegno il calcolo %d%c%d al figlio %d\n", op1, calc, op2, val);
				write(STDOUT, stampa, strlen(stampa));
			}
			
			//passo operandi e operazione al figlio
			memoria[val-1]->val1=op1;
			memoria[val-1]->op=calc;
			memoria[val-1]->val2=op2;
			
			//libero il figlio che era in attesa sul semaforo
			sops[0].sem_num = 2*(val-1);  	// semaforo val-1
        		sops[0].sem_op = -1;
		        sops[0].sem_flg = 0;
        		semop(semaforo, sops, 1);
        		
		} while(r>0);
	}
	
//###############################################################################################//
//					FIGLI							 //
//###############################################################################################//
///5- Svolgimento delle operazioni da parte dei figli
	
	else{
		//i figli non usciranno mai da questo while fino alla terminazione
		//il codice successivo al while è visibile solo dal padre
		while(true){
			
			sops[0].sem_num = 2*(id-1);
        		sops[0].sem_op = 0;  		// attende che il semaforo valga zero
        		sops[0].sem_flg = 0;
        		sops[1].sem_num = 2*(id-1);
        		sops[1].sem_op = 1;		// mette a 1 il suo semaforo così non rientra subito
        		sops[1].sem_flg = 0;
        		semop(semaforo, sops, 2);

			sleep(1);
			
			//lettura di operazione e operandi
			op1=memoria[id-1]->val1;
			calc=memoria[id-1]->op;
			op2=memoria[id-1]->val2;
			
			//svolgimento del calcolo o terminazione in caso di calc='K'
			switch(calc){
				case '+':
					memoria[id-1]->res=op1+op2;
					break;
				case '-':
					memoria[id-1]->res=op1-op2;
					break;
				case '*':
					memoria[id-1]->res=op1*op2;
					break;
				case '/':
					memoria[id-1]->res=op1/op2;
					break;
				case 'K':
					sprintf(stampa, "Figlio %d: TERMINO\n", id);
					write(STDOUT, stampa, strlen(stampa));
					exit(0);
				default:
					write(STDOUT, "Operazione non consentita\n", strlen("Operazione non consentita\n"));
			}
			memoria[id-1]->finish=true;
			sprintf(stampa, "Figlio %d: ho svolto il calcolo %d%c%d=%d\n", id, op1, calc, op2, memoria[id-1]->res);
			write(STDOUT, stampa, strlen(stampa));
			
			//segnala al padre il termine delle sue operazioni
			sops[0].sem_num = 2*(id-1)+1;
        		sops[0].sem_op = -1;
		        sops[0].sem_flg = 0;
        		semop(semaforo, sops, 1);
        		
        		//incremento il semaforo intero perchè c'è un altro figlio libero dopo il calcolo
        		sops[0].sem_num = 2*NUM_PROC;
    			sops[0].sem_op = 1;
    			sops[0].sem_flg = 0;
    			semop(semaforo, sops, 1);
			
		}
	}
	
//###############################################################################################//
//				PRELIEVO RISULTATI DA PARTE DEL PADRE				 //
//###############################################################################################//
///6- Prelievo dei risultati, salvataggio su file e invio del segnale di terminazione
	
	//il padre attenderà la terminazione dei calcoli da parte di ogni figlio per andare poi a prelevare i risultati
	for(j=0; j<NUM_PROC; j++){
		sops[0].sem_num = 2*NUM_PROC;	// P semaforo intero
    		sops[0].sem_op = -1;
    		sops[0].sem_flg = 0;
    		semop(semaforo, sops, 1);
			
		sops[0].sem_num = 2*(j)+1;
		sops[0].sem_op = 0;  		// attende che il semaforo valga zero (figlio termina i calcoli)
		sops[0].sem_flg = 0;
		sops[1].sem_num = 2*(j)+1;
		sops[1].sem_op = 1;  		// mette a 1 il suo semaforo così non rientra subito
		sops[1].sem_flg = 0;
		semop(semaforo, sops, 2);
	
			
		//controllo se il figlio ha svolto operazioni delle quali non ho ancora prelevato il risultato, in tal caso lo prelevo
		if(memoria[j]->finish==true){	
			itoa(memoria[j]->res, res);
			itoa(memoria[j]->val1, oper1);
			itoa(memoria[j]->val2, oper2);

			strcat(risultati[counter], oper1);
			strcat(risultati[counter], &(memoria[j]->op));
			strcat(risultati[counter], oper2);
			strcat(risultati[counter], "=");
			strcat(risultati[counter], res);
			strcat(risultati[counter], "\n");
			counter++;
			memoria[j]->finish=false;
			
			sprintf(stampa, "\tPADRE: ho prelevato il risultato: %s del figlio %d e gli invio il segnale di terminazione\n", risultati[counter-1], val);
			write(STDOUT, stampa, strlen(stampa));
		}
		//invio del segnale di terminazione tramite memoria condivisa
		memoria[j]->op='K';
	
		//libera il figlio che andrà a leggere il segnale K e terminerà
		sops[0].sem_num = 2*(j);
        	sops[0].sem_op = -1;
	        sops[0].sem_flg = 0;
        	semop(semaforo, sops, 1);
       
	}	
		
///7- Attesa della terminazione di ciascun figlio
	for(j=0; j<NUM_PROC; j++)
		wait(NULL);
	write(STDOUT, "\tPADRE: i figlio sono tutti terminati\n", strlen("\tPADRE: i figlio sono tutti terminati\n"));
		
	//padre stacca la memoria condivisa dalla sua zona dati
	for(j=0; j<NUM_PROC; j++)
		shmdt(memoria[j]);
	write(STDOUT, "\tPADRE: memoria staccata\n", strlen("\tPADRE: memoria staccata\n"));
		
	//STAMPA DEI RISULTATI SU FILE
	file_name="Risultati.txt";	//nome del file da creare
	fd=creat(file_name, 0777);	//creazione del file
	if(fd==-1){
		write(STDOUT, "Errore in apertura del file\n", strlen("Errore in apertura del file\n"));
		exit(1);
	}
	//scrittura su file di ciascuna entry dell'array dei risultati
	for(j=0; j<righe; j++)
		write(fd, risultati[j], strlen(risultati[j]));
	write(STDOUT, "\tPADRE: risultati scritti su file\n", strlen("\tPADRE: risultati scritti su file\n"));
	
	//eliminazione memoria condivisa
	for(j=0; j<NUM_PROC; j++)
        	if (shmctl(shm_id[j], IPC_RMID, NULL) == -1)
            		write(STDOUT, "Non posso rimuovere la memorica condivisa\n", strlen("Non posso rimuovere la memorica condivisa\n"));
	write(STDOUT, "\tPADRE: memoria condivisa rimossa\n", strlen("\tPADRE: memoria condivisa rimossa\n"));

        //rimozione dei semafori
	if (semctl(semaforo, 0, IPC_RMID, 0) == -1)
		write(STDOUT, "I semafori non sono stati rimossi\n", strlen("I semafori non sono stati rimossi\n"));
	write(STDOUT, "\tPADRE: semafori rimossi\n", strlen("\tPADRE: semafori rimossi\n"));
		
	//libero la memoria allocata per l'array dei risultati e per le 2 operazioni sull'array dei semafori
	free(sops);
	for(j=0; j<righe; j++)
		free(risultati[j]);
        
///8- Terminazione del padre
        write(STDOUT, "\tPADRE: Termino anche io!\n", strlen("\tPADRE: Termino anche io!\n"));
	exit(0);
}





/**
 * @brief Funzione che riceve come input un array di cifre in char e ritorna il corrispondente intero.
 *La funzione trasforma ciascun char dell'array in intero e attraverso la moltiplicazione del 10
 *restituisce il rispettivo valore intero
 *
 * @param array[]	array di char
 * @param i		contatore dell'array di char
 * @return 		valore intero corrispondente alle cifre espresse in char nell'array ricevuto come input
*/
	
int calcolo_char(char array[], int i){

	int j, t, k, res=0;	//contatori e risultato
	int s=i;
	for(j=0; j<i; j++){
		t=(array[j]-'0');
		for(k=s-1; k>0; k--)
			t=t*10;
		s--;
		res+=t;
	}
	return res;
}

/**
 * @brief Funzione che riceve come input un valore intero e salva nella stringa ricevuta come input il corrispondente array di char.
 *
 *	La funzione trasforma il valore intero e attraverso la moltiplicazione del 10 e la chiamata
 *	alla funzione "reverse" restituisce il rispettivo array di char.
 *
 * @param n		valore intero
 * @param s[]		array di char nel quale verrà salvato il risultato
 *
*/

void itoa(int n, char s[]){
	int i, sign;
	if((sign=n)<0)
		n=-n;
	i=0;
	do{
		s[i++]=n%10+'0';
	}while((n/=10)>0);
	if(sign<0)
		s[i++]='-';
	s[i]='\0';
	reverse(s);
}

/**
 * @brief Funzione che riceve come input un array di char e lo modifica invertendo la posizione dei caratteri.
 *
 *	La funzione fa uso di una variabile ausiliaria per poter scambiare la posizione dei char e ottenere
 *	così la stringa invertita.
 *
 * @param s[]		array di char che verrà invertita
*/

void reverse(char s[]){
	int i, j;
	char c;
	for(i=0, j=strlen(s)-1; i<j; i++, j--){
		c=s[i];
		s[i]=s[j];
		s[j]=c;
	}
}









