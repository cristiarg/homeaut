
enum work_state_t : byte {
  /**
   * "stable" states
   */
  STATE_INVALID = 0,
  STATE_IDLE,
  STATE_UP,
  STATE_DOWN,
  /**
   * "transition" state
   * when in this state, logic will discard an input command (also
   * helps in preventing button de-bounce because)
   */
  STATE_CHANGING
};

enum remctrl_command_t : byte {
  COMMAND_NA = 0,
  COMMAND_STOP,
  COMMAND_UP,
  COMMAND_DOWN,
};