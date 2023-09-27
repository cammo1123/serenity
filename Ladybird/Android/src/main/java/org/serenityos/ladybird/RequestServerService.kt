/**
 * Copyright (c) 2023, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package org.serenityos.ladybird

import android.os.Message
import android.util.Log

class RequestServerService : LadybirdServiceBase("RequestServerService") {
    override fun handleServiceSpecificMessage(msg: Message): Boolean {
        Log.e(TAG, "RequestServerService got a message it doesn't know how to handle!")
        return false
    }

    companion object {
        init {
            System.loadLibrary("requestserver")
        }
    }
}
