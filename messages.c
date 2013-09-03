#include <stdio.h>
#include <stdlib.h>

#include "list.h"

struct mes_queue {
	struct list_head list;
	int * message;
	int avail_time;
};

void mes_queue_init(struct mes_queue * queue) {
	INIT_LIST_HEAD(&(queue->list));
}

void mes_queue_push(struct mes_queue * queue, int messtypes, int * message, int avail_time) {
	unsigned int i;
	struct mes_queue *tmp;

	tmp= (struct mes_queue *)malloc(sizeof(struct mes_queue));
	tmp->message=(int *) malloc(messtypes*sizeof(int));
	INIT_LIST_HEAD(&(tmp->list));
	for(i=0; i<messtypes; i++){
		*(tmp->message+i)=*(message+i);
	}
	tmp->avail_time=avail_time;
	list_add_tail(&(tmp->list), &(queue->list));
}

int * mes_queue_pop(struct mes_queue * queue, int messtypes) {
	int * result=NULL;
	struct list_head *pos, *q;
	struct mes_queue *tmp;

	list_for_each_safe(pos, q,  &(queue->list)){
		tmp= list_entry(pos, struct mes_queue, list);
		list_del(pos);
		result=tmp->message;
		free(tmp);
		return result;
	}
}

int * mes_queue_timedpop(struct mes_queue * queue, int messtypes,int curr_time) {
	int * result=NULL;
	struct list_head *pos, *q;
	struct mes_queue *tmp;

	list_for_each_safe(pos, q,  &(queue->list)){
		tmp= list_entry(pos, struct mes_queue, list);
		list_del(pos);
		if (tmp->avail_time<=curr_time) result=tmp->message;
		free(tmp);
		return result;
	}
}

int mes_queue_count(struct mes_queue * queue, int messtypes) {
	int result=0;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q,  &(queue->list)){
		result++;
	}
	return result;
}


void mes_queue_trav(struct mes_queue * queue, int messtypes) {
	unsigned int i;
	struct list_head *pos;
	struct mes_queue *tmp;

	list_for_each(pos, &(queue->list)){
		tmp= list_entry(pos, struct mes_queue, list);
		printf ("%d ",tmp->avail_time);
		for(i=0; i<messtypes; i++){
			if (i==0) {
				printf("(%d",*(tmp->message+i));
			} else {
				printf(" %d",*(tmp->message+i));
			}
		}
		printf(")");
	}
}

/*
int main(int argc, char **argv){
	unsigned int i;
	struct mes_queue qq;
	int * temp;

	int testmess1[3]={1,2,3};
	int testmess2[3]={5,5,6};

	mes_queue_init(&qq);

	mes_queue_push(&qq,3,testmess1,23);
	mes_queue_push(&qq,3,testmess2,2);

	mes_queue_trav(&qq,3);

	temp=mes_queue_pop(&qq,3);

	mes_queue_trav(&qq,3);
	

	for(i=0; i<3; i++){
		printf("%d ",*(temp+i));
//		tmp= (struct mes_queue *)malloc(sizeof(struct mes_queue));
//		INIT_LIST_HEAD(&tmp->list);
//		printf("enter to and from:");
//		scanf("%d %d", &tmp->to, &tmp->from);
//		list_add_tail(&(tmp->list), &(mylist.list));
//		
	}
//	printf("\n");
//
//	printf("traversing the list using list_for_each()\n");
//	list_for_each(pos, &mylist.list){
//		tmp= list_entry(pos, struct mes_queue, list);
//		printf("to= %d from= %d\n", tmp->to, tmp->from);
//	}
//
//	printf("traversing the list using list_for_each_entry()\n");
//	list_for_each_entry(tmp, &mylist.list, list)
//		 printf("to= %d from= %d\n", tmp->to, tmp->from);
//	printf("\n");
//
//	printf("deleting the list using list_for_each_safe()\n");
//	list_for_each_safe(pos, q, &mylist.list){
//		 tmp= list_entry(pos, struct mes_queue, list);
//		 printf("freeing item to= %d from= %d\n", tmp->to, tmp->from);
//		 list_del(pos);
//		 free(tmp);
//	}
//
	return 0;

}

*/
