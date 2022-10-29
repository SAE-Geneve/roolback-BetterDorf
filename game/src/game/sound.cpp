#include "game/sound.h"

#include "utils/log.h"

void SoundPlayer::Init()
{
	if (!hitBuffer_.loadFromFile("data/sounds/Hit.wav"))
	{
		core::LogError("Couldn't load hit sound");
	}
	if (!hitBigBuffer_.loadFromFile("data/sounds/HitBig.wav"))
	{
		core::LogError("Couldn't load big hit sound");
	}
	if (!winBuffer_.loadFromFile(""))
	{
		core::LogError("Couldn't load win sound");
	}
	if (!loseBuffer_.loadFromFile(""))
	{
		core::LogError("Couldn't load lose sound");
	}
}

void SoundPlayer::PlayEffectSound(SoundEffect soundEffect)
{
	// Find a sound source that isn't playing something
	bool foundSource = false;
	auto it = sounds_.begin();
	for (; it < sounds_.end(); ++it)
	{
		if (it->getStatus() == sf::SoundSource::Stopped)
		{
			foundSource = true;
			break;
		}
	}

	if (!foundSource)
	{
		sounds_.emplace_back();
		it = sounds_.end() - 1;
		it->setLoop(false);
	}

	switch (soundEffect)
	{
	case SoundEffect::HIT:
		it->setBuffer(hitBuffer_);
		break;
	case SoundEffect::HIT_BIG:
		it->setBuffer(hitBigBuffer_);
		break;
	case SoundEffect::WIN:
		it->setBuffer(winBuffer_);
		break;
	case SoundEffect::LOSE:
		it->setBuffer(loseBuffer_);
		break;
	}
	it->play(); 
}
