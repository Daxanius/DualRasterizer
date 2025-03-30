#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "MathHelpers.h"
#include "Matrix.h"
#include "Vector3.h"
#include "Timer.h"
#include <algorithm>

namespace dae {
	struct Camera {
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle, float _aspectRatio) :
			origin{ _origin },
			fovAngle{ _fovAngle },
			aspectRatio{ _aspectRatio } {
		}


		Vector3 origin{};
		float fovAngle{ 90.f };
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{};
		float totalYaw{};

		float aspectRatio{};

		float zNear{ 1.f };
		float zFar{ 100.f };

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(int screenWidth, int screenHeight, float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f });

		void CalculateViewMatrix();

		void CalculateProjectionMatrix();

		Matrix GetWorldViewProjectionMatrix() const;

		void Update(const Timer* pTimer);
	};
}
