#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include <unistd.h>
#include<queue>

using namespace std;


//semaphore to control sleep and wake up
sem_t empty_semaphore;
sem_t full_semaphore;
queue<int> q;


void init_semaphore()
{
	sem_init(&empty_semaphore,0,5);
	sem_init(&full_semaphore,0,0);
}

void * ProducerFunc(void * arg)
{	
	printf("%s\n",(char*)arg);
	int i;
	for(i=1;i<=10;i++)
	{
		sem_wait(&empty_semaphore);

			
		sleep(1);
		
		q.push(i);
		printf("producer produced item %d\n",i);
		
		
	
		sem_post(&full_semaphore);
	}
}

void * ConsumerFunc(void * arg)
{
	printf("%s\n",(char*)arg);
	int i;
	for(i=1;i<=10;i++)
	{	
		sem_wait(&full_semaphore);
 		
		sleep(1);
		

		int item = q.front();
		q.pop();
		printf("consumer consumed item %d\n",item);	

			
		sem_post(&empty_semaphore);
	}
}





int main(void)
{	
	pthread_t thread1;
	pthread_t thread2;
	
	init_semaphore();
	
	char * message1 = "i am producer";
	char * message2 = "i am consumer";	
	
	pthread_create(&thread1,NULL,ProducerFunc,(void*)message1 );
	pthread_create(&thread2,NULL,ConsumerFunc,(void*)message2 );

	while(1);
	return 0;
}
