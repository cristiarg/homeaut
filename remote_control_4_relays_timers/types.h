
enum work_state_t : byte {
  STATE_INVALID = 0,
  STATE_IDLE,
  STATE_UP,
  STATE_DOWN,
  /**
   * currently changing state; cannot take another command;
   * this also helps in preventing button de-bounce 
   */
  STATE_CHANGING
};

enum remctrl_command_t : byte {
  COMMAND_NA = 0,
  COMMAND_STOP,
  COMMAND_UP,
  COMMAND_DOWN,
};