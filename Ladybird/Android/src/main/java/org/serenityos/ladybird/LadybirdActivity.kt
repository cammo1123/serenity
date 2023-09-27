/**
 * Copyright (c) 2023, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

package org.serenityos.ladybird

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.KeyEvent
import android.view.inputmethod.EditorInfo
import android.widget.EditText
import android.widget.TextView
import org.serenityos.ladybird.databinding.ActivityMainBinding
import java.net.MalformedURLException
import java.net.URL

class LadybirdActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var resourceDir: String
    private lateinit var view: WebView
    private lateinit var urlEditText: EditText
    private var timerService = TimerExecutorService()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        resourceDir = TransferAssets.transferAssets(this)
        initNativeCode(resourceDir, "Ladybird", timerService)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        setSupportActionBar(binding.toolbar)
        urlEditText = binding.urlEditText
        view = binding.webView
        view.onLoadStart = { url: String, _ ->
            urlEditText.setText(url, TextView.BufferType.EDITABLE)
        }
        urlEditText.setOnEditorActionListener { textView: TextView?, i: Int, _: KeyEvent? ->
            Boolean
            if (i == EditorInfo.IME_ACTION_GO || i == EditorInfo.IME_ACTION_SEARCH) {
                if ((textView == null) || (textView.text == null))
                    return@setOnEditorActionListener false

                var url = textView.text.toString()
                url = try {
                    URL(url).toString()
                } catch (e: MalformedURLException) {
                    "https://$url"
                }

                view.loadURL(url)
                urlEditText.clearFocus()

                // Hide the keyboard
                val inputMethodManager = getSystemService(INPUT_METHOD_SERVICE) as? android.view.inputmethod.InputMethodManager
                inputMethodManager?.hideSoftInputFromWindow(textView.windowToken, 0)
            }
            false
        }
        view.initialize(resourceDir)
        view.loadURL("https://ladybird.dev")
    }

    override fun onStart() {
        super.onStart()
    }

    override fun onDestroy() {
        view.dispose()
        disposeNativeCode()
        super.onDestroy()
    }

    private fun scheduleEventLoop() {
        mainExecutor.execute {
            execMainEventLoop()
        }
    }

    private external fun initNativeCode(
        resourceDir: String,
        tag: String,
        timerService: TimerExecutorService
    )

    private external fun disposeNativeCode()
    private external fun execMainEventLoop()

    companion object {
        // Used to load the 'ladybird' library on application startup.
        init {
            System.loadLibrary("ladybird")
        }
    }
}
