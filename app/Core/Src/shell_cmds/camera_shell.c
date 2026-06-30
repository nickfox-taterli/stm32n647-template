#include "camera_shell.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "camera_demo.h"
#include "imx415.h"
#include "shell.h"

#define P(fmt, ...) shellPrint(shellGetCurrent(), fmt, ##__VA_ARGS__)

/* Exposure window mirrors the IMX415 driver (see imx415.h). */
#define CAM_EXP_MIN_LINES   IMX415_EXPOSURE_MIN_LINES
#define CAM_EXP_MAX_LINES   (IMX415_DEMO_VMAX_LINES - IMX415_EXPOSURE_OFFSET_LINES)
#define CAM_AGAIN_MIN       IMX415_ANALOG_GAIN_MIN
#define CAM_AGAIN_MAX       IMX415_ANALOG_GAIN_MAX
/* White-balance window, units x1000 (see camera_demo.c). */
#define CAM_WB_MIN          250U
#define CAM_WB_MAX          4000U

/* ================================================================
 *  Parsing helpers
 * ================================================================ */

/* Parse a non-negative integer. Returns 0 on success only when the whole
 * string is consumed and the value fits in uint32_t (no overflow). */
static int parse_u32(const char *s, uint32_t *out)
{
  char *end = NULL;
  unsigned long value;

  if (s == NULL)
  {
    return -1;
  }

  errno = 0;
  value = strtoul(s, &end, 0);

  if ((end == s) || (end == NULL) || (*end != '\0') || (errno == ERANGE))
  {
    return -1;
  }

  *out = (uint32_t)value;
  return 0;
}

static const char *bayer_name(CameraBayerPattern bayer)
{
  switch (bayer)
  {
    case CAMERA_BAYER_RGGB: return "rggb";
    case CAMERA_BAYER_GRBG: return "grbg";
    case CAMERA_BAYER_GBRG: return "gbrg";
    case CAMERA_BAYER_BGGR: return "bggr";
    default: return "?";
  }
}

static const char *tpg_name(IMX415_TestPattern pattern)
{
  switch (pattern)
  {
    case IMX415_TEST_PATTERN_OFF: return "off";
    case IMX415_TEST_PATTERN_HORIZONTAL_COLOR_BAR: return "hbar";
    case IMX415_TEST_PATTERN_VERTICAL_COLOR_BAR: return "vbar";
    default: return "?";
  }
}

static void cam_print_usage(void)
{
  P("Usage:\r\n");
  P("  cam status\r\n");
  P("  cam exp <lines %lu..%lu>\r\n", (unsigned long)CAM_EXP_MIN_LINES,
    (unsigned long)CAM_EXP_MAX_LINES);
  P("  cam again <%u..%u>\r\n", (unsigned int)CAM_AGAIN_MIN,
    (unsigned int)CAM_AGAIN_MAX);
  P("  cam wb <r_x1000> <g_x1000> <b_x1000>  (%lu..%lu)\r\n",
    (unsigned long)CAM_WB_MIN, (unsigned long)CAM_WB_MAX);
  P("  cam tpg off|hbar|vbar\r\n");
  P("  cam bayer rggb|grbg|gbrg|bggr\r\n");
  P("  cam swaprb <0|1>\r\n");
  P("  cam defaults\r\n");
}

/* ================================================================
 *  Subcommand handlers
 * ================================================================ */

static int cam_cmd_help(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  cam_print_usage();
  return 0;
}

static int cam_cmd_status(int argc, char **argv)
{
  CameraDemoControls active;
  CameraDemoControls pending;
  CameraDemoDebug dbg;
  IMX415_DebugRegisters sen;
  uint32_t dirty;

  (void)argc;
  (void)argv;

  CameraDemo_GetControls(&active, &pending, &dirty);
  CameraDemo_GetDebug(&dbg);
  CameraDemo_GetSensorDebug(&sen);

  P("=== camera status ===\r\n");
  P("sensor:\r\n");
  P("  exposure_lines : active=%lu pending=%lu\r\n",
    (unsigned long)active.exposure_lines, (unsigned long)pending.exposure_lines);
  P("  analog_gain    : active=%u pending=%u\r\n",
    (unsigned int)active.analog_gain, (unsigned int)pending.analog_gain);
  P("  test_pattern   : active=%s pending=%s\r\n",
    tpg_name(active.test_pattern), tpg_name(pending.test_pattern));
  P("  VMAX    [0x3024] = 0x%06lx (%lu)\r\n",
    (unsigned long)sen.vmax, (unsigned long)sen.vmax);
  P("  SHR0    [0x3050] = 0x%06lx\r\n", (unsigned long)sen.shr0);
  P("  GAIN    [0x3090] = 0x%04x\r\n", (unsigned int)sen.gain);
  P("  BLKLEVEL[0x30E2] = 0x%04x\r\n", (unsigned int)sen.blklevel);
  P("  TPG_EN  [0x30E4] = 0x%02x\r\n", (unsigned int)sen.tpg_en);
  P("  TPG_SEL [0x30E6] = 0x%02x\r\n", (unsigned int)sen.tpg_sel);

  P("isp:\r\n");
  P("  white balance  : active=%lu/%lu/%lu pending=%lu/%lu/%lu\r\n",
    (unsigned long)active.wb_r_x1000, (unsigned long)active.wb_g_x1000,
    (unsigned long)active.wb_b_x1000,
    (unsigned long)pending.wb_r_x1000, (unsigned long)pending.wb_g_x1000,
    (unsigned long)pending.wb_b_x1000);
  P("  bayer          : active=%s pending=%s\r\n",
    bayer_name(active.bayer), bayer_name(pending.bayer));
  P("  swap_rb        : active=%u pending=%u\r\n",
    (unsigned int)active.swap_rb, (unsigned int)pending.swap_rb);
  P("  P1DMCR  = 0x%08lx\r\n", (unsigned long)dbg.dcmipp_p1dmcr);
  P("  P1EXCR1 = 0x%08lx\r\n", (unsigned long)dbg.dcmipp_p1excr1);
  P("  P1EXCR2 = 0x%08lx\r\n", (unsigned long)dbg.dcmipp_p1excr2);
  P("  P1PPCR  = 0x%08lx\r\n", (unsigned long)dbg.dcmipp_p1ppcr);
  P("  P1DECR  = 0x%08lx\r\n", (unsigned long)dbg.dcmipp_p1decr);

  P("capture:\r\n");
  P("  CSI SR0  = 0x%08lx\r\n", (unsigned long)dbg.csi_sr0);
  P("  CSI ERR1 = 0x%08lx\r\n", (unsigned long)dbg.csi_err1);
  P("  CSI ERR2 = 0x%08lx\r\n", (unsigned long)dbg.csi_err2);
  P("  P1SR     = 0x%08lx\r\n", (unsigned long)dbg.dcmipp_p1sr);
  P("  fb range = 0x%04x .. 0x%04x\r\n",
    (unsigned int)dbg.fb_min, (unsigned int)dbg.fb_max);

  P("dirty mask = 0x%02lx\r\n", (unsigned long)dirty);
  return 0;
}

static int cam_cmd_exp(int argc, char **argv)
{
  uint32_t lines;

  if (argc < 2)
  {
    CameraDemoControls active;
    CameraDemoControls pending;

    CameraDemo_GetControls(&active, &pending, NULL);
    P("exp: active=%lu pending=%lu\r\n",
      (unsigned long)active.exposure_lines, (unsigned long)pending.exposure_lines);
    return 0;
  }

  if (parse_u32(argv[1], &lines) != 0)
  {
    P("Usage: cam exp <lines %lu..%lu>\r\n",
      (unsigned long)CAM_EXP_MIN_LINES, (unsigned long)CAM_EXP_MAX_LINES);
    return -1;
  }

  CameraDemoStatus status = CameraDemo_SetExposureLines(lines);
  if (status != CAMERA_DEMO_OK)
  {
    P("cam exp: rejected (status=%u)\r\n", (unsigned int)status);
    return -1;
  }

  P("cam exp %lu queued (applied next frame)\r\n", (unsigned long)lines);
  return 0;
}

static int cam_cmd_again(int argc, char **argv)
{
  uint32_t gain;

  if (argc < 2)
  {
    CameraDemoControls active;
    CameraDemoControls pending;

    CameraDemo_GetControls(&active, &pending, NULL);
    P("again: active=%u pending=%u\r\n",
      (unsigned int)active.analog_gain, (unsigned int)pending.analog_gain);
    return 0;
  }

  if (parse_u32(argv[1], &gain) != 0)
  {
    P("Usage: cam again <%u..%u>\r\n",
      (unsigned int)CAM_AGAIN_MIN, (unsigned int)CAM_AGAIN_MAX);
    return -1;
  }

  /* Reject before the uint16_t cast so a value such as 65586 cannot wrap into
   * the valid window. The setter re-validates the same range. */
  if (gain > CAM_AGAIN_MAX)
  {
    P("cam again: out of range (got %lu, max %u)\r\n",
      (unsigned long)gain, (unsigned int)CAM_AGAIN_MAX);
    return -1;
  }

  CameraDemoStatus status = CameraDemo_SetAnalogGain((uint16_t)gain);
  if (status != CAMERA_DEMO_OK)
  {
    P("cam again: rejected (status=%u)\r\n", (unsigned int)status);
    return -1;
  }

  P("cam again %lu queued (applied next frame)\r\n", (unsigned long)gain);
  return 0;
}

static int cam_cmd_wb(int argc, char **argv)
{
  uint32_t r;
  uint32_t g;
  uint32_t b;

  if (argc < 2)
  {
    CameraDemoControls active;
    CameraDemoControls pending;

    CameraDemo_GetControls(&active, &pending, NULL);
    P("wb: active=%lu/%lu/%lu pending=%lu/%lu/%lu\r\n",
      (unsigned long)active.wb_r_x1000, (unsigned long)active.wb_g_x1000,
      (unsigned long)active.wb_b_x1000,
      (unsigned long)pending.wb_r_x1000, (unsigned long)pending.wb_g_x1000,
      (unsigned long)pending.wb_b_x1000);
    return 0;
  }

  if ((argc < 4) ||
      (parse_u32(argv[1], &r) != 0) ||
      (parse_u32(argv[2], &g) != 0) ||
      (parse_u32(argv[3], &b) != 0))
  {
    P("Usage: cam wb <r_x1000> <g_x1000> <b_x1000>  (%lu..%lu)\r\n",
      (unsigned long)CAM_WB_MIN, (unsigned long)CAM_WB_MAX);
    return -1;
  }

  /* Reject before the uint16_t casts so values above 65535 cannot wrap into the
   * valid window. The setter re-validates the full [min,max] range. */
  if ((r > CAM_WB_MAX) || (g > CAM_WB_MAX) || (b > CAM_WB_MAX))
  {
    P("cam wb: out of range (got %lu/%lu/%lu, max %lu)\r\n",
      (unsigned long)r, (unsigned long)g, (unsigned long)b,
      (unsigned long)CAM_WB_MAX);
    return -1;
  }

  CameraDemoStatus status = CameraDemo_SetWhiteBalance((uint16_t)r,
                                                       (uint16_t)g,
                                                       (uint16_t)b);
  if (status != CAMERA_DEMO_OK)
  {
    P("cam wb: rejected (status=%u)\r\n", (unsigned int)status);
    return -1;
  }

  P("cam wb %lu/%lu/%lu queued (applied next frame)\r\n",
    (unsigned long)r, (unsigned long)g, (unsigned long)b);
  return 0;
}

static int cam_cmd_tpg(int argc, char **argv)
{
  IMX415_TestPattern pattern;

  if (argc < 2)
  {
    CameraDemoControls active;
    CameraDemoControls pending;

    CameraDemo_GetControls(&active, &pending, NULL);
    P("tpg: active=%s pending=%s\r\n",
      tpg_name(active.test_pattern), tpg_name(pending.test_pattern));
    return 0;
  }

  if (strcmp(argv[1], "off") == 0)
  {
    pattern = IMX415_TEST_PATTERN_OFF;
  }
  else if (strcmp(argv[1], "hbar") == 0)
  {
    pattern = IMX415_TEST_PATTERN_HORIZONTAL_COLOR_BAR;
  }
  else if (strcmp(argv[1], "vbar") == 0)
  {
    pattern = IMX415_TEST_PATTERN_VERTICAL_COLOR_BAR;
  }
  else
  {
    P("cam tpg: unknown pattern '%s'\r\n", argv[1]);
    P("Usage: cam tpg off|hbar|vbar\r\n");
    return -1;
  }

  CameraDemoStatus status = CameraDemo_SetTestPattern(pattern);
  if (status != CAMERA_DEMO_OK)
  {
    P("cam tpg: rejected (status=%u)\r\n", (unsigned int)status);
    return -1;
  }

  P("cam tpg %s queued (applied next frame)\r\n", tpg_name(pattern));
  return 0;
}

static int cam_cmd_bayer(int argc, char **argv)
{
  CameraBayerPattern pattern;

  if (argc < 2)
  {
    CameraDemoControls active;
    CameraDemoControls pending;

    CameraDemo_GetControls(&active, &pending, NULL);
    P("bayer: active=%s pending=%s\r\n",
      bayer_name(active.bayer), bayer_name(pending.bayer));
    return 0;
  }

  if (strcmp(argv[1], "rggb") == 0)
  {
    pattern = CAMERA_BAYER_RGGB;
  }
  else if (strcmp(argv[1], "grbg") == 0)
  {
    pattern = CAMERA_BAYER_GRBG;
  }
  else if (strcmp(argv[1], "gbrg") == 0)
  {
    pattern = CAMERA_BAYER_GBRG;
  }
  else if (strcmp(argv[1], "bggr") == 0)
  {
    pattern = CAMERA_BAYER_BGGR;
  }
  else
  {
    P("cam bayer: unknown pattern '%s'\r\n", argv[1]);
    P("Usage: cam bayer rggb|grbg|gbrg|bggr\r\n");
    return -1;
  }

  CameraDemoStatus status = CameraDemo_SetBayerPattern(pattern);
  if (status != CAMERA_DEMO_OK)
  {
    P("cam bayer: rejected (status=%u)\r\n", (unsigned int)status);
    return -1;
  }

  P("cam bayer %s queued (applied next frame)\r\n", bayer_name(pattern));
  return 0;
}

static int cam_cmd_swaprb(int argc, char **argv)
{
  uint32_t enable;

  if (argc < 2)
  {
    CameraDemoControls active;
    CameraDemoControls pending;

    CameraDemo_GetControls(&active, &pending, NULL);
    P("swaprb: active=%u pending=%u\r\n",
      (unsigned int)active.swap_rb, (unsigned int)pending.swap_rb);
    return 0;
  }

  if (parse_u32(argv[1], &enable) != 0)
  {
    P("Usage: cam swaprb <0|1>\r\n");
    return -1;
  }

  if ((enable != 0U) && (enable != 1U))
  {
    P("cam swaprb: must be 0 or 1 (got %lu)\r\n", (unsigned long)enable);
    return -1;
  }

  CameraDemoStatus status = CameraDemo_SetSwapRB((uint8_t)enable);
  if (status != CAMERA_DEMO_OK)
  {
    P("cam swaprb: rejected (status=%u)\r\n", (unsigned int)status);
    return -1;
  }

  P("cam swaprb %lu queued (applied next frame)\r\n", (unsigned long)enable);
  return 0;
}

static int cam_cmd_defaults(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  CameraDemoStatus status = CameraDemo_RestoreDefaults();
  if (status != CAMERA_DEMO_OK)
  {
    P("cam defaults: rejected (status=%u)\r\n", (unsigned int)status);
    return -1;
  }

  P("cam defaults queued (applied next frame)\r\n");
  return 0;
}

/* ================================================================
 *  Subcommand dispatch
 * ================================================================ */

typedef struct
{
  const char *name;
  int (*fn)(int argc, char **argv);
} cam_subcmd_t;

static const cam_subcmd_t cam_subcmds[] = {
  { "help",    cam_cmd_help },
  { "status",  cam_cmd_status },
  { "exp",     cam_cmd_exp },
  { "again",   cam_cmd_again },
  { "wb",      cam_cmd_wb },
  { "tpg",     cam_cmd_tpg },
  { "bayer",   cam_cmd_bayer },
  { "swaprb",  cam_cmd_swaprb },
  { "defaults",cam_cmd_defaults },
  { NULL,      NULL },
};

int cam_cmd(int argc, char **argv)
{
  if (argc < 2)
  {
    cam_print_usage();
    return 0;
  }

  const char *sub = argv[1];

  for (const cam_subcmd_t *c = cam_subcmds; c->name != NULL; c++)
  {
    if (strcmp(sub, c->name) == 0)
    {
      return c->fn(argc - 1, &argv[1]);
    }
  }

  P("cam: unknown subcommand '%s'\r\n", sub);
  cam_print_usage();
  return -1;
}

SHELL_EXPORT_CMD(SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),
                 cam,
                 cam_cmd,
                 camera runtime debug: status|exp|again|wb|tpg|bayer|swaprb|defaults);
