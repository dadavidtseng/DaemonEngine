Audio Module
============

The Audio Module provides comprehensive audio management capabilities through FMOD integration, supporting both 2D and 3D spatial audio with real-time playback control. This module delivers high-performance audio processing suitable for interactive applications and games.

Overview
--------

The Audio Module primarily provides the following functionality:

* **Sound Resource Management** - Efficient loading, caching, and cleanup of audio assets
* **2D Audio Playback** - Non-positional audio for UI sounds, music, and ambient audio
* **3D Spatial Audio** - Positional audio with distance attenuation and listener tracking
* **Real-time Audio Control** - Dynamic volume, balance, speed, and position adjustments
* **FMOD Integration** - Professional audio engine integration with error handling

AudioSystem Class
------------------

High-level audio management system providing FMOD-based sound loading and playback capabilities. Manages sound resource lifecycle, 3D spatial audio, and real-time playback control through a comprehensive API.

Key Features:

* **Dual Audio Modes** - Supports both 2D (UI/music) and 3D (positioned) audio playback
* **Resource Caching** - Automatic sound file caching to prevent duplicate loading
* **Multiple Playback** - Simultaneous playback of same sound with independent control
* **Spatial Audio** - Full 3D positioning with listener orientation and distance attenuation
* **Professional Integration** - Direct FMOD system access for advanced audio features

.. doxygenclass:: AudioSystem
   :members:
   :undoc-members:

Audio System Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: sAudioSystemConfig
   :members:

Audio Types and Enumerations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sound Identifiers
^^^^^^^^^^^^^^^^^

.. doxygentypedef:: SoundID

.. doxygentypedef:: SoundPlaybackID

.. doxygendefine:: MISSING_SOUND_ID

Audio Dimensionality
^^^^^^^^^^^^^^^^^^^^

.. doxygenenum:: eAudioSystemSoundDimension

Usage Examples
~~~~~~~~~~~~~~

Basic Audio Playback
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   #include "Engine/Audio/AudioSystem.hpp"
   
   // Initialize audio system
   sAudioSystemConfig config;
   AudioSystem audioSystem(config);
   audioSystem.Startup();
   
   // Load and play 2D sound
   SoundID buttonSound = audioSystem.CreateOrGetSound("assets/ui/button_click.wav", 
                                                      eAudioSystemSoundDimension::Sound2D);
   SoundPlaybackID playback = audioSystem.StartSound(buttonSound, false, 0.8f);
   
   // Check if still playing
   if (audioSystem.IsPlaying(playback)) {
       audioSystem.StopSound(playback);
   }

3D Spatial Audio
^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Load 3D sound for spatial audio
   SoundID engineSound = audioSystem.CreateOrGetSound("assets/vehicles/engine.ogg", 
                                                       eAudioSystemSoundDimension::Sound3D);
   
   // Setup 3D listener (typically camera position)
   audioSystem.SetNumListeners(1);
   audioSystem.UpdateListener(0, cameraPos, cameraForward, cameraUp);
   
   // Start 3D sound at vehicle position
   Vec3 vehiclePosition(100.0f, 0.0f, 50.0f);
   SoundPlaybackID vehicleAudio = audioSystem.StartSoundAt(engineSound, vehiclePosition, 
                                                           true, 10.0f);  // looped
   
   // Update sound position as vehicle moves (call every frame)
   audioSystem.SetSoundPosition(vehicleAudio, updatedVehiclePosition);

Real-time Audio Control
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Dynamic volume control for music
   SoundID musicTrack = audioSystem.CreateOrGetSound("assets/music/background.mp3", 
                                                     eAudioSystemSoundDimension::Sound2D);
   SoundPlaybackID musicPlayback = audioSystem.StartSound(musicTrack, true, 1.0f);
   
   // Fade music volume over time
   float currentVolume = 1.0f;
   while (currentVolume > 0.0f) {
       currentVolume -= 0.01f;  // Fade step
       audioSystem.SetSoundPlaybackVolume(musicPlayback, currentVolume);
       // Wait for next frame...
   }
   
   // Change playback speed for slow-motion effect
   audioSystem.SetSoundPlaybackSpeed(musicPlayback, 0.5f);  // Half speed
   
   // Adjust stereo balance
   audioSystem.SetSoundPlaybackBalance(musicPlayback, -0.5f);  // Favor left channel

Frame-based Audio Management
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Typical game loop integration
   void GameLoop() {
       while (gameRunning) {
           // Update audio system state (call every frame)
           audioSystem.BeginFrame();
           
           // Update 3D listener position with camera
           audioSystem.UpdateListener(0, camera.GetPosition(), 
                                     camera.GetForward(), camera.GetUp());
           
           // Update moving sound sources
           for (auto& vehicle : vehicles) {
               if (vehicle.IsEngineRunning()) {
                   audioSystem.SetSoundPosition(vehicle.GetEngineSound(), 
                                               vehicle.GetPosition());
               }
           }
           
           // Complete frame processing
           audioSystem.EndFrame();
           
           // ... rest of game logic
       }
       
       // Cleanup on exit
       audioSystem.Shutdown();
   }

Error Handling and Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // FMOD error validation
   void SafeAudioOperation() {
       FMOD_RESULT result = FMOD_OK;
       
       // Perform FMOD operation
       result = fmodSystem->createSound("sound.wav", FMOD_DEFAULT, 0, &sound);
       
       // Validate result
       audioSystem.ValidateResult(result);
       
       // Handle invalid sound IDs
       SoundID soundID = audioSystem.CreateOrGetSound("nonexistent.wav", 
                                                     eAudioSystemSoundDimension::Sound2D);
       if (soundID == MISSING_SOUND_ID) {
           // Handle failed sound loading
           Logger::LogError("Failed to load sound file");
       }
   }

Performance Considerations
~~~~~~~~~~~~~~~~~~~~~~~~~

Audio Optimization Best Practices:

* **Resource Management** - Use CreateOrGetSound() caching to avoid duplicate loading
* **3D Audio Limits** - Monitor active 3D sounds to prevent performance degradation  
* **Frame Updates** - Call BeginFrame()/EndFrame() consistently for optimal FMOD performance
* **Memory Usage** - Large audio files may require streaming or background loading
* **Error Checking** - Use ValidateResult() to catch FMOD errors early

Integration Notes
~~~~~~~~~~~~~~~~~

FMOD Integration:

* **Version Compatibility** - Designed for FMOD 2.02+ API
* **Threading** - Thread-safe for most operations, FMOD handles internal synchronization
* **Platform Support** - Cross-platform through FMOD's multi-platform architecture
* **Codec Support** - Supports WAV, MP3, OGG, and other FMOD-compatible formats

Related Resources
-----------------

* :doc:`../api` - Back to API Overview
* :doc:`core_module` - Core Module Documentation  
* :doc:`../quickstart` - Quick Start Guide
* `FMOD Documentation <https://fmod.com/docs/2.02/api/>`_ - Official FMOD API Reference
