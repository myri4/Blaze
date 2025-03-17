#include <wc/Utils/Log.h>

#include "miniaudio.h"

#include <string>
#include <map>

namespace Audio
{
	// Used to configure the SoundContext's sound engine upon initialization.
	struct EngineConfig
	{
#if !defined(MA_NO_DEVICE_IO)
		ma_device_id* pPlaybackDeviceID = ma_engine_config_init().pPlaybackDeviceID;	/* The ID of the playback device to use with the default listener. */
#endif
		ma_uint32 listenerCount = ma_engine_config_init().listenerCount;				/* Must be between 1 and MA_ENGINE_MAX_LISTENERS. */
		ma_uint32 channels = ma_engine_config_init().channels;							/* The number of channels to use when mixing and spatializing. When set to 0, will use the native channel count of the device. */
		ma_uint32 sampleRate = ma_engine_config_init().sampleRate;						/* The sample rate. When set to 0 will use the native channel count of the device. */
	};

	// Used to configure sounds upon initialization.
	// Can and should be reused.
	struct SoundConfig
	{
	private:
		uint32_t flags = 0;
	public:
		// Returns the flags of the SoundConfig.
		// @return this->flags
		uint32_t GetFlags() const { return this->flags; }

		//  Adds flags to the flags used on initialization of a sound that uses this config.
		//
		//  List of flags:
		// 
		//	MA_SOUND_FLAG_STREAM
		// 
		//	MA_SOUND_FLAG_DECODE
		// 
		//	MA_SOUND_FLAG_ASYNC
		// 
		//	MA_SOUND_FLAG_NO_PITCH
		// 
		//	MA_SOUND_FLAG_NO_SPATIALIZATION
		// @param flags - the flags to be added.
		void AddFlags(uint32_t flags) { this->flags |= flags; }

		// Removes flags from the flags used on initialization of a sound that uses this config.
		//
		//  List of flags:
		// 
		//	MA_SOUND_FLAG_STREAM
		// 
		//	MA_SOUND_FLAG_DECODE
		// 
		//	MA_SOUND_FLAG_ASYNC
		// 
		//	MA_SOUND_FLAG_NO_PITCH
		// 
		//	MA_SOUND_FLAG_NO_SPATIALIZATION
		// @param flags - the flags to be removed.
		void RemoveFlags(uint32_t flags) { this->flags &= ~flags; }

		// Returns a SoundConfig object with the default settings.
		static SoundConfig SoundConfigDefaults()
		{
			SoundConfig config;

			return config;
		}
	};

	// Contains the needed information to distinguish each sound instance and the SoundConfig used to initialize each specific sound instance.
	// Used to reference specific sound instances.
	struct SoundID
	{
		std::string name;
		uint32_t id;
		bool operator == (const SoundID& other) const { return this->name == other.name && this->id == other.id; }
		bool operator < (const SoundID& other) const { return this->name < other.name || (this->name == other.name && this->id < other.id); }
	};

	// Contains the file location of the sound that will be needed for loading it.
	// Contains the instanceCount which is used internally but can be returned to the user as well.
	struct Sound
	{
		std::string name;
		uint32_t instanceCount = 0;
	};

	// The main structure used for handling sounds.
	// Initialize with an EngineConfig if you want to modify the default settings.
	struct SoundContext
	{
	private:
		ma_engine engine;
		std::map<uint32_t, Sound> sounds;
		std::map<SoundID, ma_sound*> soundInstances;
		std::map<uint32_t, ma_sound_group*> soundGroups;
		uint32_t soundCount = 0;
	public:
		// Initializes a SoundContext to manage sounds with.
		// Must call before doing anything else with the SoundContext.
		// @param EngineConfig - modifies the default SoundContext's engine settings.
		void InitializeContext(const EngineConfig* config = NULL)
		{
			ma_engine_config _config = ma_engine_config_init();
			if (config != NULL)
			{
				_config.pPlaybackDeviceID = config->pPlaybackDeviceID;
				_config.listenerCount = config->listenerCount;
				_config.channels = config->channels;
				_config.sampleRate = config->sampleRate;
			}
			ma_engine_init(&_config, &engine);
			WC_DEBUG("Successfuly initialized context");
		}

		// Uninitializes the SoundContext and all of it's members.
		// Every handle returned from the SoundContext will not be usable after uninitializing it.
		void UninitializeContext()
		{
			for (auto sound : soundInstances)
				ma_sound_uninit(sound.second);
			sounds.clear();
			soundInstances.clear();
			soundCount = 0;
			ma_engine_uninit(&engine);
			WC_DEBUG("Successfuly uninitialized context");
		}

		// Saves the file path needed to load the sound later on.
		// Changing the file location of the sound after loading it with this function will result in an invalid sound.
		// If you want to change the file location of the sound unload it first and then load it again with the new file path.
		// @param soundPath - the file location of the sound you want to load.
		// @return uint32_t - a handle to be used for referencing the sound in the SoundContext later on.
		uint32_t LoadSound(const std::string& soundPath)
		{
			uint32_t counter = 0;
			auto it = sounds.find(counter);
			while (it != sounds.end())
			{
				counter += 1;
				it = sounds.find(counter);
			}
			sounds.insert({ counter, { soundPath, 0 } });
			soundCount += 1;
			WC_DEBUG("Successfuly loaded sound number {}: {}", counter, soundPath);
			return counter;
		}

		// Unloads the sound. Will unload all sound instances of this sound as well.
		// You will not be able to reference this sound or it's instances again with the handles you got when loading it and it's instances.
		// @param soundIndex - the index of the sound that will be unloaded.
		void UnloadSound(uint32_t soundIndex)
		{
			sounds.erase(soundIndex);
			UninitializeSound(soundIndex);
			soundCount -= 1;
			WC_DEBUG("Successfuly unloaded sound number {}: {}", soundIndex, sounds.at(soundIndex).name);
		}

		// Reloads the sound and all of it's instances with a new file location.
		// 
		// WARNING!
		// 
		// IF THE NEW FILE LOCATION LEADS TO A DIFFERENT SOUND FILE THAN THE ONE USED INITIALLY THE FUNCTION WILL CAUSE A CRASH!
		// @param soundIndex - the index of the sound that will be reloaded.
		// @param soundPath - the new file location of the sound.
		void ReloadSound(uint32_t soundIndex, const std::string& soundPath)
		{
			sounds.at(soundIndex).name = soundPath;
			uint32_t counter = sounds.at(soundIndex).instanceCount;
			SoundID sID;
			sID.name = sounds.at(soundIndex).name;
			sID.id = 0;
			auto it = soundInstances.find(sID);
			while (counter > 0)
			{
				if (it != soundInstances.end())
				{
					ReloadSoundInstance(sID);
					counter -= 1;
				}
				sID.id += 1;
				it = soundInstances.find(sID);
			}
			WC_DEBUG("Successfuly reloaded sound number {}: {}", soundIndex, soundPath);
		}

		// Initializes the sound instance of the sound referenced by the soundIndex and returns a SoundID to reference the newly created sound instance.
		// @param soundIndex - the index of the sound from which the sound instance will be created.
		// @param config - a pointer to the SoundConfig object used to set the initial settings of the sound instance.
		// @return SoundID - the id of the sound instance.
		SoundID InitializeSoundInstance(uint32_t soundIndex, const SoundConfig* config = NULL)
		{
			SoundID sID;
			sID.name = sounds.at(soundIndex).name;
			sID.id = 0;
			auto it = soundInstances.find(sID);
			while (it != soundInstances.end())
			{
				sID.id += 1;
				it = soundInstances.find(sID);
			}
			ma_sound* sound = new ma_sound;
			uint32_t flags = config == NULL ? 0 : config->GetFlags();
			ma_sound_init_from_file(&engine, sounds.at(soundIndex).name.c_str(), flags, NULL, NULL, sound);
			soundInstances.insert({ sID, sound });
			sounds.at(soundIndex).instanceCount += 1;
			WC_DEBUG("Successfuly initialized a sound instance of sound number {} with id: {}", soundIndex, sID.id);
			return sID;
		}

		// Uninitializes the sound instance.
		// You will not be able to reference this sound instance after uninitializing it.
		// @param sID - the id of the sound instance.
		void UninitializeSoundInstance(SoundID sID)
		{
			ma_sound_uninit(soundInstances.at(sID));
			soundInstances.erase(sID);
			uint32_t soundIndex = 0;
			for (auto it : sounds) if (it.second.name == sID.name) soundIndex = it.first;
			sounds.at(soundIndex).instanceCount -= 1;
			WC_DEBUG("Successfuly uninitialized sound instance of sound number {} with id: {}", soundIndex, sID.id);
		}

		// Uninitializes all sound instances of a sound.
		// You will not be able to reference any of the sound instances of this sound with their SoundID's after uninitializing them.
		void UninitializeSound(uint32_t soundIndex)
		{
			uint32_t counter = sounds.at(soundIndex).instanceCount;
			SoundID sID;
			sID.name = sounds.at(soundIndex).name;
			sID.id = 0;
			auto it = soundInstances.find(sID);
			while (counter > 0)
			{
				if (it != soundInstances.end())
				{
					ma_sound_uninit(soundInstances.at(sID));
					soundInstances.erase(sID);
					counter -= 1;
				}
				sID.id += 1;
				it = soundInstances.find(sID);
			}
			sounds.at(soundIndex).instanceCount = 0;
			WC_DEBUG("Successfuly uninitialized all instances of sound number {}", soundIndex);
		}

		// Reloads a sound instance.
		// @param sID - the id of the sound instance you want to reload.
		// @param config - a pointer to the SoundConfig you want to use when reloading the sound instance.
		void ReloadSoundInstance(SoundID sID, const SoundConfig* config = NULL)
		{
			bool isPlaying = ma_sound_is_playing(soundInstances.at(sID));
			uint32_t soundIndex;
			for (auto it : sounds) if (it.second.name == sID.name) soundIndex = it.first;
			uint64_t cursor;
			uint32_t flags = config == NULL ? 0 : config->GetFlags();
			ma_sound_stop(soundInstances.at(sID));
			ma_sound_get_cursor_in_pcm_frames(soundInstances.at(sID), &cursor);
			ma_sound_uninit(soundInstances.at(sID));
			soundInstances.erase(sID);
			ma_sound* sound = new ma_sound;
			ma_sound_init_from_file(&engine, sounds.at(soundIndex).name.c_str(), flags, NULL, NULL, sound);
			ma_sound_seek_to_pcm_frame(sound, cursor);
			if (isPlaying) ma_sound_start(sound);
			soundInstances.insert({ sID, sound });
			WC_DEBUG("Successfuly reloaded sound instance of sound number {} with id: {}", soundIndex, sID.id);
		}

		// Initializes a sound group that can be used to group sound instances.
		// @return uint32_t - a handle that can be used to reference the sound group.
		uint32_t InitializeSoundGroup()
		{
			ma_sound_group* group = new ma_sound_group;
			ma_sound_group_init(&engine, 0, NULL, group);
			uint32_t counter = 0;
			auto it = soundGroups.find(counter);
			while (it != soundGroups.end())
			{
				counter += 1;
				it = soundGroups.find(counter);
			}
			soundGroups.insert({ counter, group });
			WC_DEBUG("Successfully initialized sound group with index {}", counter);
			return counter;
		}

		// Uninitializes the sound group.
		// All of the sounds from the sound group will not be a part of any sound group and will output directly to the sound context.
		// @param groupIndex - the index of the sound group that will be uninitialized.
		void UninitializeSoundGroup(uint32_t groupIndex)
		{
			ma_sound_group_uninit(soundGroups.at(groupIndex));
			soundGroups.erase(groupIndex);
			WC_DEBUG("Successfully uninitialized sound group with index {}", groupIndex);
		}

		// Adds a sound instance to the sound group.
		// @param sID - the id of the sound.
		// @param groupIndex - the index of the sound group.
		void AddInstanceToGroup(SoundID sID, uint32_t groupIndex)
		{
			ma_node_attach_output_bus(soundInstances.at(sID), 0, soundGroups.at(groupIndex), 0);
			WC_DEBUG("Successfuly added sound instance with id {} to sound group {}", sID.id, groupIndex);
		}

		// Plays the sound instance.
		// @param sID - the id of the sound instance that will be played.
		void PlaySound(SoundID sID)
		{
			ma_sound_start(soundInstances.at(sID));
			WC_DEBUG("Successfuly played sound instance with id: {}", sID.id);
		}

		// Pauses the sound instance.
		// @param sID - the id of the sound instance that will be paused.
		void PauseSound(SoundID sID)
		{
			ma_sound_stop(soundInstances.at(sID));
			WC_DEBUG("Successfuly paused sound instance with id: {}", sID.id);
		}

		// Sets the volume of the sound instance.
		// @param sID - the id of the sound instance whose volume will be changed.
		// @param volume - the volume to set for the sound instance.
		void SetVolume(SoundID sID, float volume)
		{
			ma_sound_set_volume(soundInstances.at(sID), volume < 0 ? 0 : volume);
			WC_DEBUG("Successfuly set the volume of sound instance with id: {} to {}", sID.id, volume < 0 ? 0 : volume);
		}

		// Returns the volume of the sound instance.
		// @param sID - the id of the sound instance whose volume will be returned.
		// @return float - the volume of the sound instance.
		float GetVolume(SoundID sID) { return ma_sound_get_volume(soundInstances.at(sID)); }


		// Getters

		// Returns the number of sound instances of the specified sound.
		// @param soundIndex - the index of the sound whose instance count will be returned.
		// @return uint32_t - the sound instance count.
		uint32_t GetInstanceCount(uint32_t soundIndex) { return sounds.at(soundIndex).instanceCount; }

		// Returns the number of loaded sounds.
		// Note that this is not the total number of sound instances but only the number of loaded file locations to sound files.
		// @return uint32_t - the number of loaded sounds.
		uint32_t GetSoundCount() { return soundCount; }
	};
}