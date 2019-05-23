/* 
 * Copyright © 2008 Jérôme Glisse
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Jérôme Glisse <glisse@freedesktop.org>
 */
#ifndef RADEON_TRACK_H
#define RADEON_TRACK_H

struct radeon_track_event {
    struct radeon_track_event   *next;
    char                        *file;
    char                        *func;
    char                        *op;
    unsigned                    line;
};

struct radeon_track {
    struct radeon_track         *next;
    struct radeon_track         *prev;
    unsigned                    key;
    struct radeon_track_event   *events;
};

struct radeon_tracker {
    struct radeon_track         tracks; 
};

void radeon_track_add_event(struct radeon_track *track,
                            const char *file,
                            const char *func,
                            const char *op,
                            unsigned line);
struct radeon_track *radeon_tracker_add_track(struct radeon_tracker *tracker,
                                              unsigned key);
void radeon_tracker_remove_track(struct radeon_tracker *tracker,
                                 struct radeon_track *track);
void radeon_tracker_print(struct radeon_tracker *tracker,
                          FILE *file);

#endif
