/* $Id: Hello.d $ */
/** @file
 * VBoxDTrace - Hello world sample.
 */


/* This works by matching the dtrace:::BEGIN probe, printing the greeting and
   then quitting immediately. */
BEGIN {
    trace("Hello VBox World!");
    exit(0);
}

