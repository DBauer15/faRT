#define STDOUT_RESET    "\033[0m"
#define STDOUT_RED      "\033[31m"
#define STDOUT_YELLOW   "\033[33m"
#define STDOUT_GREEN    "\033[32m"

#define LOG(x) std::cout << x << std::endl;
#define WARN(x) std::cout << STDOUT_YELLOW << x << STDOUT_RESET << std::endl;
#define ERR(x) std::cout << STDOUT_RED << x << STDOUT_RESET << std::endl;
#define SUCC(x) std::cout << STDOUT_GREEN << x << STDOUT_RESET << std::endl;
