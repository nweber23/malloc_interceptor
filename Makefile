MAKEFLAGS += -s

NAME = interceptor.so
TEST_PROG = test/test_app

SRCS = src/interceptor.c
OBJS = $(SRCS:.c=.o)

CC = cc
CFLAGS = -Wall -Wextra -Werror -fPIC
LDFLAGS = -ldl -pthread

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) -shared -o $(NAME) $(OBJS) -ldl

$(TEST_PROG): test/test.c
	$(CC) -o $(TEST_PROG) test/test.c -pthread

test: $(NAME) $(TEST_PROG)
	@echo "=== Running basic test ==="
	LD_PRELOAD=./$(NAME) ./$(TEST_PROG)
	@echo "\n=== Running test with MALLOC_FAIL_STATS ==="
	LD_PRELOAD=./$(NAME) MALLOC_FAIL_STATS=1 ./$(TEST_PROG) 2>&1 | tail -20
	@echo "\n=== Running test with MALLOC_FAIL_AT=1 (should fail first malloc) ==="
	LD_PRELOAD=./$(NAME) MALLOC_FAIL_AT=1 ./$(TEST_PROG) 2>&1 | head -20

clean:
	$(RM) $(OBJS) $(TEST_PROG)

fclean: clean
	$(RM) $(NAME)

re:
	$(MAKE) fclean
	$(MAKE) all

.PHONY: all clean fclean re test