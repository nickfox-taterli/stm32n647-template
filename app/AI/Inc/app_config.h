#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "stm32n6xx_hal.h"
#include "stai_network.h"

/* This project has one 1024x600 RGB framebuffer.  The original 993 demo
 * used two 480/800x480 layers; those values are not valid for this board. */
#define LCD_BG_WIDTH                        1024
#define LCD_BG_HEIGHT                       600
#define LCD_FG_WIDTH                        1024
#define LCD_FG_HEIGHT                       600

#define CAMERA_MIRROR_FLIP                  CMW_MIRRORFLIP_MIRROR

#define NN_WIDTH                            STAI_NETWORK_IN_1_WIDTH
#define NN_HEIGHT                           STAI_NETWORK_IN_1_HEIGHT
/* Model input, independent of the LCD framebuffer size. */
#define NN_FORMAT                           DCMIPP_PIXEL_PACKER_FORMAT_RGB888_YUV444_1
#define NN_BPP                              STAI_NETWORK_IN_1_CHANNEL
#define NN_CLASSES                          80
#define NN_CLASSES_TABLE                    {\
                                                "person",\
                                                "bicycle",\
                                                "car",\
                                                "motorcycle",\
                                                "airplane",\
                                                "bus",\
                                                "train",\
                                                "truck",\
                                                "boat",\
                                                "traffic light",\
                                                "fire hydrant",\
                                                "stop sign",\
                                                "parking meter",\
                                                "bench",\
                                                "bird",\
                                                "cat",\
                                                "dog",\
                                                "horse",\
                                                "sheep",\
                                                "cow",\
                                                "elephant",\
                                                "bear",\
                                                "zebra",\
                                                "giraffe",\
                                                "backpack",\
                                                "umbrella",\
                                                "handbag",\
                                                "tie",\
                                                "suitcase",\
                                                "frisbee",\
                                                "skis",\
                                                "snowboard",\
                                                "sports ball",\
                                                "kite",\
                                                "baseball bat",\
                                                "baseball glove",\
                                                "skateboard",\
                                                "surfboard",\
                                                "tennis racket",\
                                                "bottle",\
                                                "wine glass",\
                                                "cup",\
                                                "fork",\
                                                "knife",\
                                                "spoon",\
                                                "bowl",\
                                                "banana",\
                                                "apple",\
                                                "sandwich",\
                                                "orange",\
                                                "broccoli",\
                                                "carrot",\
                                                "hot dog",\
                                                "pizza",\
                                                "donut",\
                                                "cake",\
                                                "chair",\
                                                "couch",\
                                                "potted plant",\
                                                "bed",\
                                                "dining table",\
                                                "toilet",\
                                                "tv",\
                                                "laptop",\
                                                "mouse",\
                                                "remote",\
                                                "keyboard",\
                                                "cell phone",\
                                                "microwave",\
                                                "oven",\
                                                "toaster",\
                                                "sink",\
                                                "refrigerator",\
                                                "book",\
                                                "clock",\
                                                "vase",\
                                                "scissors",\
                                                "teddy bear",\
                                                "hair drier",\
                                                "toothbrush"\
                                            }
#define NN_OUTPUT_NUMBER                    STAI_NETWORK_OUT_NUM

#define POSTPROCESS_TYPE                    POSTPROCESS_ISEG_YOLO_V8_UI
#define AI_YOLOV8_SEG_PP_TOTAL_BOXES        STAI_NETWORK_OUT_1_CHANNEL
#define AI_YOLOV8_SEG_PP_NB_CLASSES         NN_CLASSES
#define AI_YOLOV8_SEG_PP_MASK_NB            STAI_NETWORK_OUT_2_CHANNEL
#define AI_YOLOV8_SEG_PP_MASK_SIZE          STAI_NETWORK_OUT_2_HEIGHT
#define AI_YOLOV8_SEG_PP_CONF_THRESHOLD     0.5f
#define AI_YOLOV8_SEG_PP_IOU_THRESHOLD      0.4f
#define AI_YOLOV8_SEG_PP_MAX_BOXES_LIMIT    10

#endif
