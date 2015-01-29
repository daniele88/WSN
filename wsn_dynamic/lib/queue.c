#include "queue.h"

void insert(packet_t *pointer, uint8_t* data, uint8_t next_hop)
{
	/* Iterate through the list till we encounter the last node.*/
	while(pointer->next!=NULL)
	{
		pointer = pointer -> next;
	}
	/* Allocate memory for the new node and put data in it.*/
	pointer->next = (packet_t *)malloc(sizeof(packet_t));
	pointer = pointer->next;
	//FIXME check if it's correct!!! &data[1] = data
	pointer->data = data;
	pointer->data[0]=PACKET_SIZE;
	pointer->data[3]=next_hop;
	pointer->next = NULL;
}

int find(packet_t *pointer, uint8_t* key)
{
	pointer =  pointer -> next; //First node is dummy node.
	/* Iterate through the entire linked list and search for the key. */
	while(pointer!=NULL)
	{
		if(pointer->data == key) //key is found.
		{
			return 1;
		}
		pointer = pointer -> next;//Search in the next node.
	}
	/*Key is not found */
	return 0;
}

void my_delete(packet_t *pointer, int src, int id)
{
	/* Go to the node for which the node next to it has to be deleted */
	while(pointer->next!=NULL)
	{
		if ((pointer->next)->data[1] == src && (pointer->next)->data[2] < id) //check if the packet source and id are right
		{
			/* Now pointer points to a node and the node next to it has to be removed */
			packet_t *temp;
			temp = pointer -> next;
			/*temp points to the node which has to be removed*/
			pointer->next = temp->next;
			/*We removed the node which is next to the pointer (which is also temp) */
			free(temp);
			/* Beacuse we deleted the node, we no longer require the memory used for it .
           free() will deallocate the memory.
			 */
		}
		pointer = pointer -> next;
	}
	return;
}
