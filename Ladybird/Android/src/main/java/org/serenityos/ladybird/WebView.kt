/**
 * Copyright (c) 2023, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package org.serenityos.ladybird

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.util.AttributeSet
import android.util.Log
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
import android.view.ViewConfiguration
import android.widget.Scroller
import androidx.core.view.GestureDetectorCompat
import androidx.core.view.NestedScrollingChildHelper
import androidx.core.view.ScrollingView
import kotlin.math.max
import kotlin.math.min

// FIXME: This should (eventually) implement NestedScrollingChild3 and ScrollingView

class WebView(context: Context, attributeSet: AttributeSet?) : View(context, attributeSet), ScrollingView {

    private val childHelper: NestedScrollingChildHelper = NestedScrollingChildHelper(this)
    private val scroller: Scroller = Scroller(context)

    private val touchSlop: Int = ViewConfiguration.get(context).scaledTouchSlop
    private val viewImpl = WebViewImplementation(this)

    private var pageX: Int = 0
    private var pageY: Int = 0
    private var pageWidth: Int = 0
    private var pageHeight: Int = 0

    private val gestureDetector: GestureDetectorCompat

    init {
        isNestedScrollingEnabled = true
        gestureDetector = GestureDetectorCompat(context, WebViewGestureListener(this))
    }
    
    override fun computeHorizontalScrollExtent(): Int {
        return width
    }
    override fun computeVerticalScrollExtent(): Int {
        return height
    }

    override fun computeHorizontalScrollOffset(): Int {
        return pageX
    }
    override fun computeVerticalScrollOffset(): Int {
        return pageY
    }
    override fun computeHorizontalScrollRange(): Int {
        return pageWidth - width
    }
    override fun computeVerticalScrollRange(): Int {
        return pageHeight - height
    }
    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        setMeasuredDimension(pageWidth, pageHeight)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (gestureDetector.onTouchEvent(event))
            return true
        return true
    }

    private lateinit var contentBitmap: Bitmap
    var onLoadStart: (url: String, isRedirect: Boolean) -> Unit = { _, _ -> }
    var onLoadFinish: () -> Unit = {}
    var onLinkClick: (url: String) -> Unit = {}

    fun initialize(resourceDir: String) {
        viewImpl.initialize(resourceDir)
    }

    fun dispose() {
        viewImpl.dispose()
    }

    fun loadURL(url: String) {
        viewImpl.loadURL(url)
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        contentBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)

        val pixelDensity = context.resources.displayMetrics.density
        viewImpl.setDevicePixelRatio(pixelDensity)

        // FIXME: Account for scroll offset when view supports scrolling
        viewImpl.setViewportGeometry(this.pageX, this.pageY, w, h)
    }

    fun addScrollOffset(dx: Int, dy: Int) {
        this.pageX = min(max(0, this.pageX + dx) , pageWidth - width)
        this.pageY = min(max(0, this.pageY + dy), pageHeight - height)
        viewImpl.setViewportGeometry(this.pageX, this.pageY, width, height);
    }

    fun setMouseDown(x: Int, y: Int) {
        viewImpl.setMouseDown(this.pageX + x, this.pageY + y)
    }

    fun setMouseUp(x: Int, y: Int) {
        viewImpl.setMouseUp(this.pageX + x, this.pageY + y)
    }

    override fun onDraw(canvas: Canvas?) {
        super.onDraw(canvas)

        viewImpl.drawIntoBitmap(contentBitmap);
        canvas?.drawBitmap(contentBitmap, 0f, 0f, null)
    }

    fun onDidLayout(w: Int, h: Int) {
        pageWidth = w
        pageHeight = h
    }

    class WebViewGestureListener(private val webView: WebView) : GestureDetector.SimpleOnGestureListener() {
        override fun onDown(event: MotionEvent): Boolean {
            webView.setMouseDown(event.x.toInt(), event.y.toInt())
            return true
        }

        override fun onShowPress(event: MotionEvent) {
            Log.d("WebViewGestureListener", "onShowPress $event")
        }

        override fun onSingleTapUp(event: MotionEvent): Boolean {
            webView.setMouseUp(event.x.toInt(), event.y.toInt())
            return true
        }

        override fun onScroll(start: MotionEvent, current: MotionEvent, dx: Float, dy: Float): Boolean {
            webView.addScrollOffset(dx.toInt(), dy.toInt())
            return true
        }

        override fun onLongPress(event: MotionEvent) {
            Log.d("WebViewGestureListener", "onLongPress $event")
        }

        override fun onFling(start: MotionEvent, current: MotionEvent, p2: Float, p3: Float): Boolean {
            Log.d("WebViewGestureListener", "onFling $start $current $p2 $p3")
            return true
        }

    }
}
