
#ifndef LIBMILLWEED_LIST_H
#define LIBMILLWEED_LIST_H


#ifdef __cplusplus
extern "C" {
#endif

typedef struct list_s list_t;

//should return 0 upon successful match, 1 upon failed match, and -1 upon error
typedef int (*comparator_func)(void *element1, void *element2);

int void_ptr_comparator(void *element1, void *element2);
int char_ptr_comparator(void *element1, void *element2);

void list_create_dontalloc(list_t *list);
list_t *list_create();
void list_destroy_dontfreeelements(list_t *list);
void list_destroy(list_t *list);

unsigned long list_count(list_t *list);
void list_add(list_t *list, void *element);
int list_add_unique(list_t *list, void *element);
int list_add_unique_with_comparator(list_t *list, void *element, comparator_func comparator);
void *list_get(list_t *list, unsigned long index);
int list_find(list_t *list, void *element);
int list_find_with_comparator(list_t *list, void *element, comparator_func comparator);

#ifdef __cplusplus
}
#endif


#endif
