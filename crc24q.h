/*
* Copyright (C) 2010 Swift Navigation Inc.
* Contact: Fergus Noble <fergus@swift-nav.com>
*
* This source is subject to the license found in the file 'LICENSE' which must
* be be distributed together with this source. All other rights reserved.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef CRC24Q_H
#define CRC24Q_H

#include <stdint.h>

extern uint32_t crc24q(uint32_t crc, uint32_t start, uint32_t len);

#endif /* CRC24Q_H */
