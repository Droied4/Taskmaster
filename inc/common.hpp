#define CONFIG_ERROR(prog, msg)                                                \
  do {                                                                         \
    std::cerr << "Config Error [" << prog << "]: " << msg << "\n";             \
    std::exit(1);                                                              \
  } while (0)

#define ERROR(msg)                                                             \
  do {                                                                         \
    Logs::error() << msg << std::endl;                                         \
    exit(1);                                                                   \
  } while (0)

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::cerr << "SYSTEM ASSERT FAILED: " << message << "\n"                 \
                << "File: " << __FILE__ << " Line: " << __LINE__ << "\n";      \
      std::abort();                                                            \
    }                                                                          \
  } while (0)
