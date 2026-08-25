#include "venc_recorder.h"
#include "shell.h"

#include <string.h>

static const char *venc_state_name(VencRecorderState state)
{
  switch (state)
  {
    case VENC_RECORDER_IDLE: return "idle";
    case VENC_RECORDER_REQUESTED: return "requested";
    case VENC_RECORDER_RECORDING: return "recording";
    case VENC_RECORDER_COMPLETE: return "complete";
    case VENC_RECORDER_ERROR: return "error";
    default: return "unknown";
  }
}

static int venc_cmd(int argc, char **argv)
{
  Shell *shell = shellGetCurrent();
  VencRecorderStatus status;

  if ((argc == 2) && (strcmp(argv[1], "record") == 0))
  {
    if (VencRecorder_Request() != 0)
    {
      shellPrint(shell, "VENC busy or SD NAND unavailable\r\n");
      return -1;
    }
    shellPrint(shell,
               "VENC: 10-second recording requested; SD NAND FAT will be overwritten\r\n");
    return 0;
  }

  if ((argc == 1) || ((argc == 2) && (strcmp(argv[1], "status") == 0)))
  {
    VencRecorder_GetStatus(&status);
    shellPrint(shell,
               "VENC: %s frames=%lu/%lu bytes=%lu elapsed=%lums error=%ld\r\n",
               venc_state_name(status.state),
               (unsigned long)status.frames,
               (unsigned long)status.target_frames,
               (unsigned long)status.bytes,
               (unsigned long)status.elapsed_ms,
               (long)status.error);
    return 0;
  }

  shellPrint(shell, "Usage: venc record | venc status\r\n");
  return -1;
}

SHELL_EXPORT_CMD(SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), venc, venc_cmd,
                 VENC recorder: record|status);
