/*
 * Copyright (C) 2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/**
 * @file  psp2_msgbox.h
 * @brief A collection of native PSVita message boxes with simple interface.
 */

/**
 * @warning Each type of message box can be used in two ways: synchronously
 * (blocking the program execution until the interaction is complete) and
 * asynchronously (allowing the program to keep running).
 * If using asynchronous versions (`msgbox_*_async`), make sure to always
 * wait for their `msgbox_*_async_done` counterparts to return `true` before
 * spawning new message boxes of any type. Otherwise, the behavior is undefined.
 */

#include <stdbool.h>

/**
 * <h2>Message box with "OK" button</h2>
 */

/** 
 * @brief Display a standard message box with "OK" button. Synchronous.
 *
 * Returns after the user actively terminates the message box.
 * Calls `msgbox_ok_async` and `msgbox_ok_async_done` (with `swap_buffers` set
 * to `true`) internally.
 * 
 * @param[in] fmt C string with message box contents. Up to six line breaks.
 *                Supports printf-style format specifiers.
 * @param[in] ... (Optional) Additional arguments to match format specifiers.
 */
void msgbox_ok(const char* fmt, ...) __attribute__ (( format (printf, 1, 2) ));

/** 
 * @brief Display a standard message box with "OK" button. Asynchronous.
 *
 * Returns immediately after the message box is created.
 * To check on the status of the message box, call `msgbox_ok_async_done`.
 *
 * @param[in] fmt C string with message box contents. Up to six line breaks.
 *                Supports printf-style format specifiers.
 * @param[in] ... (Optional) Additional arguments to match format specifiers.
 */
void msgbox_ok_async(const char* fmt, ...) __attribute__ (( format (printf, 1, 2) ));

/**
 * @brief Get the status of the asynchronously created message box with "OK" button.
 *
 * @param[in] swap_buffers Whether to swap buffers ("force new frame") after
 *                         checking the message box status.
 *
 * @return `true` if the process is complete, `false` otherwise.
 */
bool msgbox_ok_async_done(bool swap_buffers);

/**
 * <h2>Message box with "Yes"/"No" buttons<h2>
 */

/** 
 * @brief Display a dialog message box with "Yes"/"No" buttons. Synchronous.
 *
 * Returns after the user actively terminates the message box.
 * Calls `msgbox_yesno_async` and `msgbox_yesno_async_done` (with `swap_buffers`
 * set to `true`) internally.
 *
 * @param[in] fmt C string with message box contents. Up to six line breaks.
 *                Supports printf-style format specifiers.
 * @param[in] ... (Optional) Additional arguments to match format specifiers.
 *
 * @return `true` if the user pressed "Yes", `false` otherwise.
 */
bool msgbox_yesno(const char* fmt, ...) __attribute__ (( format (printf, 1, 2) ));

/**
 * @brief Display a dialog message box with "Yes"/"No" buttons. Asynchronous.
 *
 * Returns immediately after the message box is created.
 * To check on the status of the message box, call `msgbox_yesno_async_done`.
 *
 * @param[in] fmt C string with message box contents. Up to six line breaks.
 *                Supports printf-style format specifiers.
 * @param[in] ... (Optional) Additional arguments to match format specifiers.
 */
void msgbox_yesno_async(const char* fmt, ...) __attribute__ (( format (printf, 1, 2) ));

/**
 * @brief Get the status of the asynchronously created dialog with "Yes"/"No" buttons.
 *
 * @param[in]  swap_buffers Whether to swap buffers ("force new frame") after
 *                          checking the message box status.
 * @param[out] result       When `true` is returned, the user choice will be
 *                          written at this pointer (`true` for pressing the
 *                          "Yes" button, `false` otherwise).
 *
 * @return `true` if the process is complete, `false` otherwise.
 */
bool msgbox_yesno_async_done(bool swap_buffers, bool * result);

/**
 * <h2>Message box with 3 custom buttons</h2>
 */

/**
 * @brief Display a dialog message box with 3 custom buttons. Synchronous.
 *
 * Returns after the user actively terminates the message box.
 * Calls `msgbox_3buttons_async` and `msgbox_3buttons_async_done` (with
 * `swap_buffers` set to `true`) internally.
 *
 * @param[in] btn1 C string with a label for the first button.
 * @param[in] btn2 C string with a label for the second button.
 * @param[in] btn3 C string with a label for the third button.
 * @param[in] fmt C string with message box contents. Up to six line breaks.
 *                Supports printf-style format specifiers.
 * @param[in] ... (Optional) Additional arguments to match format specifiers.
 *
 * @return `0` if the user closed the message box; `1`, `2`, `3` if the user
 *         pressed the respective button.
 */
short int msgbox_3buttons(const char* btn1, const char* btn2, const char * btn3, const char* fmt, ...) __attribute__ (( format (printf, 4, 5) ));

/**
 * @brief Display a dialog message box with 3 custom buttons. Asynchronous.
 *
 * Returns immediately after the message box is created.
 * To check on the status of the message box, call `msgbox_3buttons_async_done`.
 *
 * @param[in] btn1 C string with a label for the first button.
 * @param[in] btn2 C string with a label for the second button.
 * @param[in] btn3 C string with a label for the third button.
 * @param[in] fmt C string with message box contents. Up to six line breaks.
 *                Supports printf-style format specifiers.
 * @param[in] ... (Optional) Additional arguments to match format specifiers.
 */
void msgbox_3buttons_async(const char* btn1, const char* btn2, const char * btn3, const char* fmt, ...) __attribute__ (( format (printf, 4, 5) ));

/**
 * @brief Get the status of the asynchronously created dialog with 3 custom buttons.
 *
 * @param[in]  swap_buffers Whether to swap buffers ("force new frame") after
 *                          checking the message box status.
 * @param[out] result       When `true` is returned, the user choice will be
 *                          written at this pointer (`0` if the user closed
 *                          the message box; `1`, `2`, `3` if the user
 *                          pressed the respective button).
 *
 * @return `true` if the process is complete, `false` otherwise.
 */
bool msgbox_3buttons_async_done(bool swap_buffers, short int * result);

/**
 * <h2>Message box with progress bar</h2>
 */

/**
 * @brief Display a message box with progress bar
 *
 * Returns immediately after the message box is created.
 *
 * @param[in] fmt (Optional) C string with message box contents.
 *                Up to six line breaks. Supports printf-style format
 *                specifiers. Can be set to NULL, in which case a default
 *                system message ("Please wait.") is displayed.
 * @param[in] ... (Optional) Additional arguments to match format specifiers.
 */
void msgbox_progress_async(const char* fmt, ...) __attribute__ (( format (printf, 1, 2) ));

/**
 * @brief Increment the value of the running message box with progress bar
 *
 * Increases the progress value of the message box by `delta` while smoothly
 * animating the change.
 *
 * @param[in] delta Increment value (from 0 to 100).
 * @param[in] fmt (Optional) C string with message box contents.
 *                Up to six line breaks. Supports printf-style format
 *                specifiers. Can be set to NULL, in which case a default
 *                system message ("Please wait.") is displayed.
 * @param[in] ... (Optional) Additional arguments to match format specifiers.
 */
void msgbox_progress_async_increment(unsigned int delta, const char* fmt, ...) __attribute__ (( format (printf, 2, 3) ));

/**
 * @brief Set the value of the running message box with progress bar
 *
 * Sets the progress value of the message box to `value`, immediately updating
 * the display (without animating).
 *
 * @param[in] value Target progress value (from 0 to 100).
 * @param[in] fmt (Optional) C string with message box contents.
 *                Up to six line breaks. Supports printf-style format
 *                specifiers. Can be set to NULL, in which case a default
 *                system message ("Please wait.") is displayed.
 * @param[in] ... (Optional) Additional arguments to match format specifiers.
 */
void msgbox_progress_async_set(unsigned int value, const char* fmt, ...) __attribute__ (( format (printf, 2, 3) ));

/**
 * @brief Close the message box with progress bar
 */
void msgbox_progress_async_done();
