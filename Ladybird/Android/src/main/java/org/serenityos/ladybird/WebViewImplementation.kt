/**
 * Copyright (c) 2023, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package org.serenityos.ladybird

import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.graphics.Bitmap
import android.graphics.Rect
import android.util.Log
import android.view.View
import java.net.URL

/**
 * Wrapper around WebView::ViewImplementation for use by Kotlin
 */
class WebViewImplementation(private val view: WebView) {
    // Instance Pointer to native object, very unsafe :)
    private var nativeInstance: Long = 0
    private lateinit var resourceDir: String
    private lateinit var connection: ServiceConnection

    fun initialize(resourceDir: String) {
        this.resourceDir = resourceDir
        nativeInstance = nativeObjectInit()
    }

    fun dispose() {
        nativeObjectDispose(nativeInstance)
        nativeInstance = 0
    }

    fun loadURL(url: String) {
        nativeLoadURL(nativeInstance, url)
    }

    fun drawIntoBitmap(bitmap: Bitmap) {
        nativeDrawIntoBitmap(nativeInstance, bitmap)
    }

    fun setViewportGeometry(x: Int, y: Int, w: Int, h: Int) {
        nativeSetViewportGeometry(nativeInstance, x, y, w, h)
    }

    fun addScrollOffset(x: Int, y: Int) {
        nativeAddScrollOffset(nativeInstance, x, y)
    }

    fun setMouseDown(x: Int, y: Int) {
        nativeSetMouseDown(nativeInstance, x, y)
    }

    fun setMouseUp(x: Int, y: Int) {
        nativeSetMouseUp(nativeInstance, x, y)
    }

    fun setDevicePixelRatio(ratio: Float) {
        nativeSetDevicePixelRatio(nativeInstance, ratio)
    }

    // Functions called from native code
    fun bindWebContentService(ipcFd: Int, fdPassingFd: Int) {
        val connector = LadybirdServiceConnection(ipcFd, fdPassingFd, resourceDir)
        connector.onDisconnect = {
            // FIXME: Notify impl that service is dead and might need restarted
            Log.e("WebContentView", "WebContent Died! :(")
        }
        // FIXME: Unbind this at some point maybe
        view.context.bindService(
            Intent(view.context, WebContentService::class.java),
            connector,
            Context.BIND_AUTO_CREATE
        )
        connection = connector
    }

    fun invalidateLayout() {
        view.requestLayout()
        view.invalidate()
    }

    fun onLoadStart(url: String, isRedirect: Boolean) {
        view.onLoadStart(url, isRedirect)
    }

    fun onLoadFinish() {
        view.onLoadFinish()
    }

    fun onLinkClick(url: String) {
        view.onLinkClick(url)
    }

    fun onDidLayout(w: Int, h: Int) {
        view.onDidLayout(w, h)
    }

    // Functions implemented in native code
    private external fun nativeObjectInit(): Long
    private external fun nativeObjectDispose(instance: Long)

    private external fun nativeDrawIntoBitmap(instance: Long, bitmap: Bitmap)
    private external fun nativeSetViewportGeometry(instance: Long, x: Int, y: Int, w: Int, h: Int)
    private external fun nativeAddScrollOffset(intance: Long, x: Int, y: Int)
    private external fun nativeSetMouseDown(instance: Long, x: Int, y: Int)
    private external fun nativeSetMouseUp(instance: Long, x: Int, y: Int)
    private external fun nativeSetDevicePixelRatio(instance: Long, ratio: Float)
    private external fun nativeLoadURL(instance: Long, url: String)

    companion object {
        /*
         * We use a static class initializer to allow the native code to cache some
         * field offsets. This native function looks up and caches interesting
         * class/field/method IDs. Throws on failure.
         */
        private external fun nativeClassInit()

        init {
            nativeClassInit()
        }
    }
};
