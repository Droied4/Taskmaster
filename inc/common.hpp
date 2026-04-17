#define CONFIG_ERROR(prog, msg, error)                                         \
  do {                                                                         \
    error = "[Config] " + std::string(prog) + ": " + std::string(msg);         \
    return false;                                                              \
  } while (0)

#define ERROR(msg)                                                             \
  do {                                                                         \
    Logs::error() << msg << std::endl;                                         \
    std::abort();                                                              \
  } while (0)

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    if (!(condition)) {                                                        \
      Logs::error() << "SYSTEM ASSERT FAILED: " << message << std::endl             \
                    << "File: " << __FILE__ << " Line: " << __LINE__ << std::endl;  \
      std::abort();                                                            \
    }                                                                          \
  } while (0)
