
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <millweed/misc.h>
#include <millweed/list.h>

#define LIST_SIZE_INCREMENT 50

struct list_s {
  void **elements;

  long unsigned count;
  long unsigned size;

  int is_alloced;
};

int void_ptr_comparator(void *element1, void *element2)
{
  if(element1 == NULL) {
    error2("element1 is NULL, returning\n");
    return -1;
  }
  if(element2 == NULL) {
    error2("element2 is NULL, returning\n");
    return -1;
  }

  return (element1 == element2)? 0: 1;
}

int char_ptr_comparator(void *element1, void *element2)
{
  if(element1 == NULL) {
    error2("element1 is NULL, returning\n");
    return -1;
  }
  if(element2 == NULL) {
    error2("element2 is NULL, returning\n");
    return -1;
  }

  //TODO: case insensitive?  dunno, look into it
  return (strcmp((const char *)element1, (const char *)element2) == 0)? 0: 1;
}

void list_create_dontalloc(list_t *list)
{
  if(list == NULL) {
    error2("list is NULL, returning\n");
    return;
  }

  memset(list, 0, sizeof(list_t));
}

list_t *list_create()
{
  list_t *list;

  list= malloc(sizeof(list_t));
  if(list == NULL) {
    error2("malloc() returned NULL, returning NULL\n");
    return NULL;
  }

  list_create_dontalloc(list);
  list->is_alloced= 1;

  return list;
}

void list_destroy_dontfreeelements(list_t *list)
{
  if(list == NULL) {
    error2("list is NULL, returning\n");
    return;
  }

  free(list->elements);  //list->elements has always been alloced
  if(list->is_alloced)
    free(list);
}

void list_destroy(list_t *list)
{
  int i;

  if(list == NULL) {
    error2("list is NULL, returning\n");
    return;
  }

  for(i=list_count(list)-1; i >= 0; i--)
    free(list->elements[i]);

  list_destroy_dontfreeelements(list);
}

unsigned long list_count(list_t *list)
{
  if(list == NULL) {
    error2("list is NULL, returning 0\n");
    return 0;
  }

  return list->count;
}

void list_check_size(list_t *list, unsigned long size)
{
  if(list == NULL) {
    error2("list is NULL, returning\n");
    return;
  }

  if(size > list->size) {
    list->size += LIST_SIZE_INCREMENT;
    list->elements= realloc(list->elements, sizeof(void *) * (list->size));

    if(size > list->size)
      error2("size is STILL > list->size!\n");
  }
}

void list_add(list_t *list, void *element)
{
  if(list == NULL) {
    error2("list is NULL, returning\n");
    return;
  }
  if(element == NULL) {
    error2("element is NULL, returning\n");
    return;
  }

  list_check_size(list, list->count+1);

  list->elements[list->count++]= element;
}

int list_add_unique(list_t *list, void *element)
{
  return list_add_unique_with_comparator(list, element, void_ptr_comparator);
}

int list_add_unique_with_comparator(list_t *list, void *element, comparator_func comparator)
{
  int index;

  index= list_find_with_comparator(list, element, comparator);
  if(index != -1) {
    debug("list_find_with_comparator() found a match in the list, returning 1\n");
    return 1;
  }

  list_add(list, element);
  return 0;
}

void *list_get(list_t *list, unsigned long index)
{
  if(list == NULL) {
    error2("list is NULL, returning NULL\n");
    return NULL;
  }
  if(index < 0 || index > list->count-1) {
    error2("index is out of range, returning NULL\n");
    return NULL;
  }

  return list->elements[index];
}

int list_find_with_comparator(list_t *list, void *element, comparator_func comparator)
{
  int i;

  if(list == NULL) {
    error2("list is NULL, returning -1\n");
    return -1;
  }
  if(element == NULL) {
    error2("element is NULL, returning -1\n");
    return -1;
  }
  if(comparator == NULL) {
    error2("comparator is NULL, returning -1\n");
    return -1;
  }

  for(i=0; i<list->count; i++)
    if((*comparator)(element, list->elements[i]) == 0)
      return i;

  return -1;
}

int list_find(list_t *list, void *element)
{
  return list_find_with_comparator(list, element, void_ptr_comparator);
}
