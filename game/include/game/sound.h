#pragma once
#include <SFML/Audio/SoundBuffer.hpp>

enum class SoundEffect
{
	HIT,
	HIT_BIG,
	WIN,
	LOSE
};

class SoundPlayer
{
public:
	void Init();

	void PlayEffectSound(SoundEffect);
private:
	sf::SoundBuffer hitBuffer_;
	sf::SoundBuffer hitBigBuffer_;
	sf::SoundBuffer winBuffer_;
	sf::SoundBuffer loseBuffer_;

	std::vector<sf::Sound> sounds_;
};