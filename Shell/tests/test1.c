#include "debug.h"
#include "icsmm.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>

#define VALUE1_VALUE 53
#define VALUE2_VALUE 0xBEEFCAFEF00D

void press_to_cont() {                                                       
    printf("Press Enter to Continue");                                      
    while (getchar() != '\n')                                               
      ;                                                                     
    printf("\n");                                                           
}

void null_check(void* ptr, long size) {
    if (ptr == NULL) {                                                       
      error(                                                                   
          "Failed to allocate %lu byte(s) for an integer using ics_malloc.\n", 
          size);                                                             
      error("%s\n", "Aborting...");                                            
      assert(false);                                                           
    } else {                                                                   
      success("ics_malloc returned a non-null address: %p\n", (void *)(ptr));  
    }                                                                          
}

void payload_check(void* ptr) {
    if ((unsigned long)(ptr) % 16 != 0) {
      warn("Returned payload address is not divisble by a quadword. %p %% 16 " 
           "= %lu\n",                                                          
           (void *)(ptr), (unsigned long)(ptr) % 16);                          
    }                                                                          
}

// This is an illustrative "crazy" macro :)
#define CHECK_PRIM_CONTENTS(actual_value, expected_value, fmt_spec, name)      \
  do {                                                                         \
    if (*(actual_value) != (expected_value)) {                                 \
      error("Expected " name " to be " fmt_spec " but got " fmt_spec "\n",     \
            (expected_value), *(actual_value));                                \
      error("%s\n", "Aborting...");                                            \
      assert(false);                                                           \
    } else {                                                                   \
      success(name " retained the value of " fmt_spec " after assignment\n",   \
              (expected_value));                                               \
    }                                                                          \
  } while (0)


int main(int argc, char *argv[]) {
  // Initialize the custom allocator
  ics_mem_init();

  // Tell the user about the fields
  info("Initialized heap\n");
  press_to_cont();

  // Print out title for first test
  printf("=== Test1: Allocation test ===\n");
  // Test #1: Allocate an integer
  void *value1 = ics_malloc(64);
  null_check(value1, 64);
  payload_check(value1);
  ics_payload_print((void*)value1);
  press_to_cont();

  // Now assign a value
  printf("=== Test2: Assignment test ===\n");
  void *ptr1 = ics_malloc(112);
  null_check(ptr1, 112);
  payload_check(ptr1);
  ics_payload_print((void*)ptr1);
  press_to_cont();

  printf("=== Test3: Allocate a second variable ===\n");
  void *value2 = ics_malloc(48);
  null_check(value2, 48);
  payload_check(value2);
  ics_payload_print((void*)value2);
  press_to_cont();


  printf("=== Test4: does value1 still equal %d ===\n", VALUE1_VALUE);
  void *ptr2 = ics_malloc(64);
  null_check(ptr2, 64);
  payload_check(ptr2);
  ics_payload_print((void*)ptr2);
  press_to_cont();

  printf("=== Test4: does value1 still equal %d ===\n", VALUE1_VALUE);
  void *value3 = ics_malloc(16);
  null_check(value3, 16);
  payload_check(value3);
  ics_payload_print((void*)value3);
  press_to_cont();
  
  printf("=== Test4: does value1 still equal %d ===\n", VALUE1_VALUE);
  void *ptr3 = ics_malloc(80);
  null_check(ptr3, 80);
  payload_check(ptr3);
  ics_payload_print((void*)ptr3);
  press_to_cont();

  printf("=== Test4: does value1 still equal %d ===\n", VALUE1_VALUE);
  void *value4 = ics_malloc(3552);
  null_check(value4, 3552);
  payload_check(value4);
  ics_payload_print((void*)value4);
  press_to_cont();

  ics_freelist_print();

  // Free a variable
  printf("=== Test5: Free a block and snapshot ===\n");
  ics_free(ptr3);
  ics_freelist_print();
  press_to_cont();

  printf("=== Test5: Free a block and snapshot ===\n");
  ics_free(ptr1);
  ics_freelist_print();
  press_to_cont();

  printf("=== Test5: Free a block and snapshot ===\n");
  ics_free(ptr2);
  ics_freelist_print();
  press_to_cont();

  printf("=== Test4: does value1 still equal %d ===\n", VALUE1_VALUE);
  void *ptr5 = ics_malloc(8);
  null_check(ptr5, 8);
  payload_check(ptr5);
  ics_payload_print((void*)ptr5);
  press_to_cont();
  
  printf("=== Test4: does value1 still equal %d ===\n", VALUE1_VALUE);
  void *value6 = ics_malloc(80);
  null_check(value6, 80);
  payload_check(value6);
  ics_payload_print((void*)value6);
  press_to_cont();

  printf("=== Test4: does value1 still equal %d ===\n", VALUE1_VALUE);
  void *value7 = ics_malloc(80);
  null_check(value7, 80);
  payload_check(value7);
  ics_payload_print((void*)value7);
  press_to_cont();


  ics_freelist_print();
  press_to_cont();


  ics_mem_fini();

  return EXIT_SUCCESS;
}
