package org.fzort.games.audio;

import java.util.HashMap;

import android.content.Context;

public class AudioManager {
	static private AudioManager instance;

	private Context context;
	private HashMap<Integer, AudioClip> soundMap;

	static public AudioManager getInstance() {
		if (instance == null)
			instance = new AudioManager();
		return instance;
	}

	private AudioManager() {
		soundMap = new HashMap<Integer, AudioClip>();
	}

	public void setContext(Context c) {
		context = c;
	}

	public void startSound(int resid, boolean loop) {
		if (!soundMap.containsKey(resid))
			soundMap.put(resid, new AudioClip(context, resid));

		soundMap.get(resid).start(loop);
	}

	public void stopSound(int resid) {
		if (soundMap.containsKey(resid))
			soundMap.get(resid).stop();
	}

	public void pauseAllSounds() {
		for (AudioClip clip : soundMap.values()) {
			clip.pause();
		}
	}

	public void resumeAllSounds() {
		for (AudioClip clip : soundMap.values()) {
			clip.resume();
		}
	}

	public void stopAllSounds() {
		for (AudioClip clip : soundMap.values()) {
			clip.stop();
		}
	}
}
