package org.fzort.games.kasui;

import android.app.NativeActivity;
import android.content.res.AssetManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.util.Log;
import android.view.WindowManager;

import java.util.ArrayList;

import org.fzort.games.audio.AudioManager;

public class Kasui extends NativeActivity {
	static ArrayList<Integer> sounds;

	static {
		sounds = new ArrayList<Integer>();

        /*
		sounds.add(R.raw.opening);
		sounds.add(R.raw.menu_select);
		sounds.add(R.raw.menu_validate);
		sounds.add(R.raw.menu_back);
		sounds.add(R.raw.level_intro);
		sounds.add(R.raw.game_over);
		sounds.add(R.raw.level_completed);
		sounds.add(R.raw.countdown);
		sounds.add(R.raw.block_match);
		sounds.add(R.raw.block_miss);
        */
	};

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

		AudioManager.getInstance().setContext(this);

		curContext = this; // HACK
	}

	protected void onResume() {
		super.onResume();

		AudioManager.getInstance().resumeAllSounds();
	}

	protected void onPause() {
		super.onPause();

		AudioManager.getInstance().pauseAllSounds();
	}

	public static void onRateMeClicked() {
		try {
			curContext.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=org.fzort.games.kasui")));
		} catch (Exception e) { }
	}

	public static void startSound(int id, boolean loop) {
		if (id < 0 || id >= sounds.size())
			return;

		AudioManager.getInstance().startSound(sounds.get(id), loop);
	};

	public static void stopSound(int id) {
		if (id < 0 || id >= sounds.size())
			return;

		AudioManager.getInstance().stopSound(sounds.get(id));
	}

	public static void stopAllSounds() {
		AudioManager.getInstance().stopAllSounds();
	}
    
	private static Context curContext; // HACK
}
