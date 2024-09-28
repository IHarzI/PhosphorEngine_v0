#pragma once

#include "Core/PH_CORE.h"

#include "Containers/PH_String.h"

namespace PhosphorEngine {


	class PH_CVarParameter;

	enum class CVarFlags : uint32_t
	{
		None = 0,
		Noedit = 1 << 1,
		EditReadOnly = 1 << 2,
		Advanced = 1 << 3,

		EditCheckbox = 1 << 8,
		EditFloatDrag = 1 << 9,
	};
	class PH_CVarSystem
	{

	public:
		static PH_CVarSystem* Get();

		//pimpl
		virtual PH_CVarParameter* GetCVar(StringUtilities::StringHash hash) = 0;

		virtual double* GetFloatCVar(StringUtilities::StringHash hash) = 0;

		virtual int32_t* GetIntCVar(StringUtilities::StringHash hash) = 0;

		virtual const char* GetStringCVar(StringUtilities::StringHash hash) = 0;

		virtual void SetFloatCVar(StringUtilities::StringHash hash, double value) = 0;

		virtual void SetIntCVar(StringUtilities::StringHash hash, int32_t value) = 0;

		virtual void SetStringCVar(StringUtilities::StringHash hash, const char* value) = 0;


		virtual PH_CVarParameter* CreateFloatCVar(const char* name, const char* description, double defaultValue, double currentValue) = 0;

		virtual PH_CVarParameter* CreateIntCVar(const char* name, const char* description, int32_t defaultValue, int32_t currentValue) = 0;

		virtual PH_CVarParameter* CreateStringCVar(const char* name, const char* description, const char* defaultValue, const char* currentValue) = 0;

		virtual void DrawImguiEditor() = 0;
	};

	template<typename T>
	struct AutoCVar
	{
	protected:
		int index;
		using CVarType = T;
	};

	struct AutoCVar_Float : AutoCVar<double>
	{
		AutoCVar_Float(const char* name, const char* description, double defaultValue, CVarFlags flags = CVarFlags::None);

		double Get();
		double* GetPtr();
		float GetFloat();
		float* GetFloatPtr();
		void Set(double val);
	};

	struct AutoCVar_Int : AutoCVar<int32_t>
	{
		AutoCVar_Int(const char* name, const char* description, int32_t defaultValue, CVarFlags flags = CVarFlags::None);
		int32_t Get();
		int32_t* GetPtr();
		void Set(int32_t val);

		void Toggle();
	};

	struct AutoCVar_String : AutoCVar<std::string>
	{
		AutoCVar_String(const char* name, const char* description, const char* defaultValue, CVarFlags flags = CVarFlags::None);

		const char* Get();
		void Set(std::string&& val);
	};

};