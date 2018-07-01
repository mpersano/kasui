package org.fzort.games.audio;

import java.io.IOException;

import android.media.MediaPlayer;

import android.content.Context;
import android.content.res.AssetFileDescriptor;

public class AudioClip implements MediaPlayer.OnCompletionListener {
	private MediaPlayer mediaPlayer;
	private AssetFileDescriptor fd;

	private enum State {
		Idle,		// mediaPlayer is null
		Started,	// mediaPlayer is in the Started state
		Stopped,	// mediaPlayer is in the Stopped state
		Paused,		// mediaPlayer is in the Paused state
		Error,		// something really, really bad happened; mediaPlayer is null
	};

	private State state;

	// private boolean isLooping;

	AudioClip(Context context, int resid) {
		fd = context.getResources().openRawResourceFd(resid);
		state = State.Idle;
	}

	void start(boolean loop) {
		if (state == State.Idle) {
			initializeMediaPlayer();
			if (state == State.Error)
				return;

			prepareMediaPlayer();
			if (state == State.Error)
				return;
		} else if (state == State.Stopped) {
			prepareMediaPlayer();
			if (state == State.Error)
				return;
		} else {
			System.out.println("AudioClip: start called for state " + state);
			return;
		}

		mediaPlayer.setLooping(loop);
		mediaPlayer.start();

		state = State.Started;
	}

	void stop() {
		if (state == State.Started) {
			mediaPlayer.stop();
			state = State.Stopped;
		} else {
			System.out.println("AudioClip: start called for state " + state);
		}
	}

	void pause() {
		if (state == State.Started) {
			mediaPlayer.pause();
			state = State.Paused;
		}
	}

	void resume() {
		if (state == State.Paused) {
			mediaPlayer.start();
			state = State.Started;
		}
	}

	private void initializeMediaPlayer() {
		mediaPlayer = new MediaPlayer();
		mediaPlayer.setOnCompletionListener(this);

		try {
			mediaPlayer.setDataSource(fd.getFileDescriptor(), fd.getStartOffset(), fd.getLength());
		} catch (IOException e) {
			mediaPlayer.release();
			mediaPlayer = null;

			state = State.Error;
		}
	}

	private void prepareMediaPlayer() {
		try {
			mediaPlayer.prepare();
		} catch (IOException e) {
			mediaPlayer.release();
			mediaPlayer = null;

			state = State.Error;
		}
	}

	public void onCompletion(MediaPlayer mp) {
		if (mp == mediaPlayer) {
			mediaPlayer.stop();
			state = State.Stopped;
		}
	}
}
