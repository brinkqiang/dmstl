/*******************************************************
 *                                                     *
 *                                                     *
 * allocator.c :                                       *
 * Grum Ketema                                         *
 * August 5, 2002                                      *
 * This memory allocator is Based on the STL allocator *
 * found in The C++ Programming language Third Edition *
 * Section 19.4 (Page 567)                             *
 *                                                     *
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include<ostream.h>
#include "gallocator.h"

struct sembuf Pool::opLock_[2] = {
					0,0,0,
					0,1,SEM_UNDO
				};

struct sembuf Pool::opUnLock_[1] = { 0,-1,SEM_UNDO };

Pool::shmPool Pool::shm_;

struct __ALIGN__ {
		void* t;
};

const int OVHD = sizeof(struct __ALIGN__);
int lock(int semid)
{
	int semPid=semctl(semid,0,GETPID,0);
	if (semPid<0) {
		return(-1);
	}
	int pid = getpid();
	if (semPid==pid) {
		return(semop(semid,&(Pool::opLock_[1]),1));
	}
	return(semop(semid,&(Pool::opLock_[0]),2));
}

int unlock(int semid)
{
	return(semop(semid,&(Pool::opUnLock_[0]),1));
}

Pool::shmPool::shmPool():
	key_(0),
	path_(NULL),
	chunks_(NULL),
	segs_(0),
	segSize_(0),
	contPtr_(NULL),
	contSemId_(0)
{
	char* env = g_key_path;
	if (env) {
		path_ = strdup(env);
	} else {
		path_=strdup(".");
	}

	key_=0;
	env = g_key_base;
	if (env) {
		key_ = atol(env);
		if (key_<0) {
			key_ = 0;
		}
	}

	segs_ = 4;//ȱʡ4��chunk
	env = g_chunck_num;
	if (env) {
		segs_ = atol(env);
		if (segs_<0) {
			segs_ = 4;
		}
	}

	segSize_ = 4096;//ȱʡ1K
	env = g_chunck_size;
	if (env) {
		segSize_ = atol(env);
		//printf("segSize_=[%d]\n",segSize_);
		if (segSize_<0) {
			segSize_ = 4096;
		}
	}
	this->init_();
}

void Pool::shmPool::init_()
{
	//printf("in Pool::shmPool::init_()\n");
	int semid=0;
	int shmid=0;
	struct shmid_ds buf;
	chunks_ = new Chunk* [segs_];
	int key = key_;
	key_t ipckey;
	for (unsigned int i=0;i<segs_;i++) 
	{
		chunks_[i] = NULL;
		//Get ipckey
		ipckey = ftok(path_,key++);
		if (ipckey==(key_t)-1) {
			perror("ipckey");
			return;
		}
		//printf("ipckey=[%x]",ipckey);
		semid = semget(ipckey,1,0664|IPC_CREAT);
		if (semid <0) {
			perror("semget");
			return;
		}
		if (lock(semid)<0) {
			perror("lock");
			return;
		}
		shmid = shmget(ipckey,segSize_,0664|IPC_CREAT);
		if (shmid<0) {
			perror("shmget");
			unlock(semid);
			return;
		}
		if (shmctl(shmid,IPC_STAT,&buf)<0) {
			perror("shmctl");
			unlock(semid);
			return;
		}
		chunks_[i] = (Chunk*)shmat(shmid,NULL,0);
		//if (!buf.shm_atime) {
			//uninitilized
		chunks_[i]->init(segSize_,i);
		chunks_[i]->semid(semid);
		chunks_[i]->shmid(shmid);
		chunks_[i]->p_buf=&buf;
		//} 
		if (!i) {
			//set container pointer
			contPtr_=new ((char*)chunks_[i] + Chunk::ALIGNED(sizeof(Chunk))) Container;
		}
		chunks_[i]->semid(semid);
		chunks_[i]->shmid(shmid);
		unlock(semid);
	}
	ipckey = ftok(path_,key++);
	if (ipckey==(key_t)-1) {
		perror("ipckey");
		return;
	}
	semid = semget(ipckey,1,0664|IPC_CREAT);
	if (semid <0) {
		perror("semget");
		return;
	}
	contSemId_ = semid;
}

Pool::shmPool::~shmPool()
{
	//printf("shmPool::~shmPool()\n");
	//detach all shared memory 
	if (chunks_) 
	{
		///printf("if (chunks_) \n");
		for (unsigned int i=0;i<segs_;i++) 
		{
			if (chunks_[i]) 
			{
				//printf("chunks_[i]->shmId_=[%d] \n",chunks_[i]->shmid());
				shmdt((void*)chunks_[i]);
				//shmctl(chunks_[i]->shmid(),IPC_RMID,NULL);
			}
		}
		delete[] chunks_;
	}
}

void* Pool::shmPool::alloc(size_t size)
{
	void* m=NULL;
	size_t maxSize;

	if (size<=0) {
		return(NULL);
	}
	maxSize = segSize_ - sizeof(Pool::shmPool::Chunk);
	if (size >maxSize) {
		return(NULL);
	}

	for (unsigned int i=0;i<segs_;i++)
	{
		if (!chunks_[i]) {
			continue;
		}
		m = chunks_[i]->alloc(size);
		if (m) {
			return(m);
		}
	}
	return(NULL);
}		

void Pool::shmPool::free(void* f, size_t size)
{
	//find out where it belongs
	//return it to the free list

	if (!f) {
		return;
	}
	if (!size) {
		return;
	}

	char* v = (char *)f;
	for (unsigned int i=0; i<segs_;i++) {
		if (!chunks_[i]) {
			continue;
		}
		char* p = (char*)chunks_[i];
		if ((v>=p)&&(v<=(p+segSize_))) {
			chunks_[i]->free(v,size);
			return;
		}
	}
	return;
}

void Pool::shmPool::print(ostream& os)
{
	for (size_t i=0;i<segs_;i++) {
		if (chunks_[i]) {
			os<<"Chunk["<<i<<"]"<<endl;
			chunks_[i]->print(os);
		}
	}
}

Pool::shmPool::Chunk::Chunk(Chunk& chunk):
	shmId_(chunk.shmId_),
	semId_(chunk.semId_),
	head_(chunk.head_)
{
}

void Pool::shmPool::Chunk::init(int segSize,int setCont)
{
	int size=segSize;
	char* m =(char*)this + ALIGNED(sizeof(Chunk));
	size = segSize - ALIGNED(sizeof(Chunk));
	if (!setCont) {
		//Reserve space for the container pointer in the first chunk
		Container* c = (Container*)m;
		c->cont = NULL;
		m = m+ALIGNED(sizeof(Container))+100;
		size = size - ALIGNED(sizeof(Container)) - 100;
	} 
	head_ = (Link *)m;
	head_->size = size;
	head_->next = NULL;
	//printf("there is Pool::shmPool::Chunk::init() and head_ be inited\n");
}

size_t Pool::shmPool::Chunk::ALIGNED(size_t sz)
{
	size_t t = sz%sizeof(struct __ALIGN__);

	sz = t?(sizeof(struct __ALIGN__) -t +sz):sz;
	return((sz< sizeof(struct Link))?sizeof(struct Link):sz);
}
void Pool::shmPool::Chunk::print(ostream& os)
{
	Link* p = head_;
	int i =0;
	if (this->lock_()<0) {
		perror("print");
	}
	while (p&&p->next) {
		os<<"\tLink["<<i++<<"] size["
		  <<p->size
		  <<"] at location["
		  <<(long)p
		  <<"] next["
		  <<(long)p->next
		  <<"]"<<endl;
		p=p->next;
	}
	this->unlock_();
}
void* Pool::shmPool::Chunk::alloc(size_t size)
{
	Link* p = head_;
	Link* prev = NULL;
	Link* r=NULL;

	if (!p) {
		return(NULL);
	}

	if (this->lock_()<0) {
		perror("alloc");
		return(NULL);
	}
	size = size + OVHD;
	while (p) {
		if (p->size>=size) {
			size_t df = p->size - size;
			if (df >= sizeof(struct Link)) {
				//split
				r = (Link*)((char*)p + size);
				r->size = df;
				p->size = size;
				r->next = p->next;
				p->next = r;
			}
			//adjust & remove p
			if (!prev) {
				//then p==head
				head_ = p->next;
			} else {
				prev->next = p->next;
			}
			this->unlock_();
			
			//cout<<"Alloc["<<(long)p<<"] size["<<p->size<<"]"<<endl;
			return((void*)((char*)p+OVHD));
		}
		prev = p;
		p = p->next;

	}
	this->unlock_();
	return(NULL);
}

void Pool::shmPool::Chunk::free(void* f,size_t size)
{


	if (!f) {
		return;
	}
	if (this->lock_()<0) {
		perror("free");
		return;
	}
	Link* p = head_;
	Link* prev = NULL;
	Link* v = (Link*) ((char*)f-OVHD);
	//v->size = size;
	v->next = NULL;
	//cout<<"Free["<<(long)v<<"] size["<<v->size<<"]"<<endl;

	while (p&&p->next) {

		//unsigned long n = (unsigned long)p;
		if (((void*) p) > f) {
			//Insert it in address order to make
			//merging easier
			if (!prev) {
				head_ = v;
				head_->next = p;
			} else {
				prev->next = v;
				v->next = p;
			}
			//Merge
			this->merge_(v);
			if (prev) {
				this->merge_(prev);
			}
			break;
		}
		prev = p;
		p=p->next;	
	}
	if (!p) {
		if (!prev) {
			//head_ must be null
			head_ = v;
			v->next = NULL;
		} else {
			//Append at the end
			prev->next = v;
			v->next = NULL;
			this->merge_(prev);
		}
	}
	this->unlock_();
	return;
}

void Pool::shmPool::Chunk::merge_(Link* v)
{

	//Merge forward
	if (v->next) {
		char* endOffset = ((char*)v)+v->size;
		if (endOffset == (char*)(v->next)) {
			v->size = v->size + v->next->size;
			v->next = v->next->next;
		}
	}
	return;
}
