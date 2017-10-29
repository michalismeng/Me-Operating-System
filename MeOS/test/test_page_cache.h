#ifndef TEST_PAGE_CACHE_H_29102017
#define TEST_PAGE_CACHE_H_29102017

#include "test_base.h"
#include "../page_cache.h"
#include "../file.h"

bool test_page_cache_reserve_anonymous();
bool test_page_cache_reserve();
bool test_page_cache_find_buffer();

#endif