#pragma once

#include <SFML\Network.hpp>
#include <deque>

enum PredictionMethod { None, Linear, Quadratic, Cubic };
enum ConnState {NotReady, Ready, Disconnected};

// Fixme:: ideally we should link specific tank/character to each connection,
// rather than assuming the order in which connections are made is the same as
// the order in which tanks/characters are stored in the game.

// This could also be made int oa class, given the increased functionality.
struct Connection
{
	std::unique_ptr<sf::TcpSocket> socket;
	ConnState state = Disconnected;

	std::deque<TankMessage> message_history;
	float inter_t = 0.f; // This value goes from 0.f to 1.f during interpolation and will be accumulated based on time delta.

	// Interpolate position based on selected method and value of t. Return TankMessage with updated position.
	sf::Vector2f Interpolate(float dt) {
		// Calculate interpolation step
		float step = 0.f;
		
		if(message_history.size() > 1) // Make sure we have at least 2 messages for Linear interpolation.
			step = 1.f / ((message_history[0].time - message_history[1].time) / dt);
		else
			step = 0.f;

		// Add step to inter_t
		inter_t += step;

		// Clamp inter_t to max of 1.f
		if(inter_t > 1.f)
			inter_t = 1.f;

		if (message_history.size() > 1) { // Make sure we have at least 2 messages for Linear interpolation.
			// Calculate new position by interpolating between last two known positons.
			return (message_history[1].position + (message_history[0].position - message_history[1].position) * inter_t);
		}
		else
			return message_history[0].position; // Just return the last known position.
	}

	auto interpolate_with_prediction(float dt, PredictionMethod _method) -> sf::Vector2f 
	{
		float step = 0.f;

		if (message_history.size() > 1) // Make sure we have at least 2 messages for Linear interpolation.
			step = 1.f / ((message_history[0].time - message_history[1].time) / dt);
			//step = 1.f / ((message_history.back().time - message_history[].time) / dt);
		else
			step = 0.f;

		// Add step to inter_t
		inter_t += step;

		// Clamp inter_t to max of 1.f
		if (inter_t > 1.f)
			inter_t = 1.f;

		sf::Vector2f currentPosition = message_history.back().position;


		if (message_history.size() > 2) { // Make sure we have at least 2 messages for Linear interpolation.
			// Calculate new position by interpolating between last two known positons.
			//return (message_history[1].position + (message_history[0].position - message_history[1].position) * inter_t);

			sf::Vector2f newPosition;

			//predict the position
			switch (_method) 
			{
				case ::PredictionMethod::Linear:
					newPosition = predict_linear(dt);
					break;
				case::PredictionMethod::Quadratic:
					newPosition = predict_quad(dt);
					break;
			}

			//interpolate to the newPosition. if the position has not changed interpolate will continue to work on the old position anyway

		/*	float lerp(float v0, float v1, float t) {
				return (1 - t) * v0 + t * v1;
			}*/


			return (1 - inter_t) * currentPosition + inter_t * newPosition;
			//return (newPosition + (currentPosition - newPosition) * inter_t);
			return newPosition;
		}
		else 
		{
			return currentPosition;
		}
	}

	auto predict_linear(float _dt) -> sf::Vector2f 
	{
		//take the last 2 known positions of the player and calculate velocity. use velocity to predict what the new position will be
		//[0] is old message
		//[1] is new message
		_dt = 1.f / 60.f;
		sf::Vector2f velocity;
		sf::Vector2f delta;

		sf::Vector2f position0 = message_history[0].position;
		sf::Vector2f position1 = message_history[1].position;

		delta = position1 - position0;

		velocity = delta / _dt;
		
		sf::Vector2f newPosition;

		newPosition = position0 + (velocity * _dt);

		return newPosition;

	}
	auto predict_quad(float _dt) -> sf::Vector2f
	{
		//take last three known positions and calculate velocity. use it to predict the next position of the player
		//[0] is old message
		//[1] is new message

		sf::Vector2f velocityA;
		sf::Vector2f velocityB;
		
		/*sf::Vector2f position0 = message_history[0].position;
		sf::Vector2f position1 = message_history[1].position;
		sf::Vector2f position2 = message_history[2].position;*/
		
		sf::Vector2f position2 = message_history[0].position;
		sf::Vector2f position1 = message_history[1].position;
		sf::Vector2f position0 = message_history[2].position;



		/*velocityA = (position0 - position1) / _dt;
		velocityB = (position1 - position2)/ _dt;*/
		/*V1 = (P1 – P0) / dt;
		V2 = (P2 – P1) / dt;*/

		velocityA = (position1 - position0)/_dt;
		velocityB = (position2 - position1)/_dt;



		sf::Vector2f final = (velocityB - velocityA) / _dt;

		sf::Vector2f newPosition;

		//predictedPosition = P3 + V2 * predictionTime + 0.5 * A * predictionTime * predictionTime;
		newPosition = position2 + velocityA * _dt + 0.5f * final * _dt * _dt;

		return newPosition;



	}




};