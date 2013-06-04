#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

#define HASH_LEN 15

struct list_head list;
struct hlist_head *hlist;

struct test {
	struct list_head list;
	struct hlist_node hn;

	int a;
};

static void __link(struct test *t, int a)
{
	t->a = a;
	list_add(&t->list, &list);
	hlist_add_head(&t->hn, &hlist[a%HASH_LEN]);
}

static void __unlink(struct test *t)
{
	hlist_del(&t->hn);
	list_del(&t->list);
}

static void init_list()
{
	int i;

	INIT_LIST_HEAD(&list);
	hlist = malloc(sizeof(struct hlist_head) * HASH_LEN);
	for (i = 0; i < HASH_LEN; i ++) {
		INIT_HLIST_HEAD(&hlist[i]);
	}
}

int main()
{
	int i;
	struct test *t;
	struct hlist_node *hn;

	init_list();

	for (i = 0; i < 30; i ++) {
		t = malloc(sizeof(*t));
		__link(t, i);
	}


	printf("hlist ");
	hlist_for_each_entry(t, &hlist[2%HASH_LEN], hn) {
		printf("%d  ", t->a);
	}
	printf("\n");

	printf("list  ");
	list_for_each_entry(t, &list, list) {
		printf("%d ", t->a);
	}
	printf("\n");

	return 0;
}
