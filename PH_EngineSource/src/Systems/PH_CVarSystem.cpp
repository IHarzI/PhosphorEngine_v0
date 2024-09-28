#include "PH_CVarSystem.h"

#include <unordered_map>

#include "Containers/PH_StaticArray.h"
#include "Containers/PH_DynamicArray.h"
#include <algorithm>
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imgui_internal.h"
#include <shared_mutex>

namespace PhosphorEngine {

	enum class CVarType : char
	{
		INT,
		FLOAT,
		STRING,
	};

	class PH_CVarParameter
	{
	public:
		friend class PH_CVarSystemImpl;

		int32_t arrayIndex;

		CVarType type;
		CVarFlags flags;
		PH_String name;
		PH_String description;
	};

	template<typename T>
	struct PH_CVarStorage
	{
		T initial;
		T current;
		PH_CVarParameter* parameter;
	};

	template<typename T>
	struct PH_CVarArray
	{
		PH_CVarStorage<T>* cvars = nullptr;
		int32_t lastCVar{ 0 };

		PH_CVarArray(uint64 Size)
		{
			cvars = new PH_CVarStorage<T>[Size]();
		}

		PH_CVarStorage<T>* GetCurrentStorage(int32_t index)
		{
			return &cvars[index];
		}

		T* GetCurrentPtr(int32_t index)
		{
			return &cvars[index].current;
		};

		T GetCurrent(int32_t index)
		{
			return cvars[index].current;
		};

		void SetCurrent(const T& val, int32_t index)
		{
			cvars[index].current = val;
		}

		int Add(const T& value, PH_CVarParameter* param)
		{
			int index = lastCVar;

			cvars[index].current = value;
			cvars[index].initial = value;
			cvars[index].parameter = param;

			param->arrayIndex = index;
			lastCVar++;
			return index;
		}

		int Add(const T& initialValue, const T& currentValue, PH_CVarParameter* param)
		{
			int index = lastCVar;

			cvars[index].current = currentValue;
			cvars[index].initial = initialValue;
			cvars[index].parameter = param;

			param->arrayIndex = index;
			lastCVar++;

			return index;
		}
	};

	uint32_t Hash(const char* str)
	{
		return StringUtilities::fnv1a_32(str, strlen(str));
	}

	class PH_CVarSystemImpl : public PH_CVarSystem
	{
	public:
		PH_CVarParameter* GetCVar(StringUtilities::StringHash hash) override final;


		PH_CVarParameter* CreateFloatCVar(const char* name, const char* description, double defaultValue, double currentValue) override final;

		PH_CVarParameter* CreateIntCVar(const char* name, const char* description, int32_t defaultValue, int32_t currentValue) override final;

		PH_CVarParameter* CreateStringCVar(const char* name, const char* description, const char* defaultValue, const char* currentValue) override final;

		double* GetFloatCVar(StringUtilities::StringHash hash) override final;
		int32_t* GetIntCVar(StringUtilities::StringHash hash) override final;
		const char* GetStringCVar(StringUtilities::StringHash hash) override final;


		void SetFloatCVar(StringUtilities::StringHash hash, double value) override final;

		void SetIntCVar(StringUtilities::StringHash hash, int32_t value) override final;

		void SetStringCVar(StringUtilities::StringHash hash, const char* value) override final;

		void DrawImguiEditor() override final;

		void EditParameter(PH_CVarParameter* p, float textWidth);

		constexpr static int MAX_INT_CVARS = 1000;
		PH_CVarArray<int32_t> intCVars2{ MAX_INT_CVARS };

		constexpr static int MAX_FLOAT_CVARS = 1000;
		PH_CVarArray<double> floatCVars{ MAX_FLOAT_CVARS };

		constexpr static int MAX_STRING_CVARS = 200;
		PH_CVarArray<PH_String> stringCVars{ MAX_STRING_CVARS };


		//using templates with specializations to get the cvar arrays for each type.
		//if you try to use a type that doesnt have specialization, it will trigger a linker error
		template<typename T>
		PH_CVarArray<T>* GetCVarArray();

		template<>
		PH_CVarArray<int32_t>* GetCVarArray()
		{
			return &intCVars2;
		}
		template<>
		PH_CVarArray<double>* GetCVarArray()
		{
			return &floatCVars;
		}
		template<>
		PH_CVarArray<PH_String>* GetCVarArray()
		{
			return &stringCVars;
		}

		//templated get-set cvar versions for syntax sugar
		template<typename T>
		T* GetCVarCurrent(uint32_t namehash) {
			PH_CVarParameter* par = GetCVar(namehash);
			if (!par) {
				return nullptr;
			}
			else {
				return GetCVarArray<T>()->GetCurrentPtr(par->arrayIndex);
			}
		}

		template<typename T>
		void SetCVarCurrent(uint32_t namehash, const T& value)
		{
			PH_CVarParameter* cvar = GetCVar(namehash);
			if (cvar)
			{
				GetCVarArray<T>()->SetCurrent(value, cvar->arrayIndex);
			}
		}

		static PH_CVarSystemImpl* Get()
		{
			return static_cast<PH_CVarSystemImpl*>(PH_CVarSystem::Get());
		}

	private:

		std::shared_mutex mutex_;

		PH_CVarParameter* InitCVar(const char* name, const char* description);

		std::unordered_map<uint32_t, PH_CVarParameter> savedCVars;

		PH_DynamicArray<PH_CVarParameter*> cachedEditParameters;
	};

	double* PH_CVarSystemImpl::GetFloatCVar(StringUtilities::StringHash hash)
	{
		return GetCVarCurrent<double>(hash);
	}

	int32_t* PH_CVarSystemImpl::GetIntCVar(StringUtilities::StringHash hash)
	{
		return GetCVarCurrent<int32_t>(hash);
	}

	const char* PH_CVarSystemImpl::GetStringCVar(StringUtilities::StringHash hash)
	{
		return GetCVarCurrent<PH_String>(hash)->c_str();
	}


	PH_CVarSystem* PH_CVarSystem::Get()
	{
		static PH_CVarSystemImpl cvarSys{};
		return &cvarSys;
	}


	PH_CVarParameter* PH_CVarSystemImpl::GetCVar(StringUtilities::StringHash hash)
	{
		std::shared_lock lock(mutex_);
		auto it = savedCVars.find(hash);

		if (it != savedCVars.end())
		{
			return &(*it).second;
		}

		return nullptr;
	}

	void PH_CVarSystemImpl::SetFloatCVar(StringUtilities::StringHash hash, double value)
	{
		SetCVarCurrent<double>(hash, value);
	}

	void PH_CVarSystemImpl::SetIntCVar(StringUtilities::StringHash hash, int32_t value)
	{
		SetCVarCurrent<int32_t>(hash, value);
	}

	void PH_CVarSystemImpl::SetStringCVar(StringUtilities::StringHash hash, const char* value)
	{
		SetCVarCurrent<PH_String>(hash, value);
	}


	PH_CVarParameter* PH_CVarSystemImpl::CreateFloatCVar(const char* name, const char* description, double defaultValue, double currentValue)
	{
		std::unique_lock lock(mutex_);
		PH_CVarParameter* param = InitCVar(name, description);
		if (!param) return nullptr;

		param->type = CVarType::FLOAT;

		GetCVarArray<double>()->Add(defaultValue, currentValue, param);

		return param;
	}



	PH_CVarParameter* PH_CVarSystemImpl::CreateIntCVar(const char* name, const char* description, int32_t defaultValue, int32_t currentValue)
	{
		std::unique_lock lock(mutex_);
		PH_CVarParameter* param = InitCVar(name, description);
		if (!param) return nullptr;

		param->type = CVarType::INT;

		GetCVarArray<int32_t>()->Add(defaultValue, currentValue, param);

		return param;
	}



	PH_CVarParameter* PH_CVarSystemImpl::CreateStringCVar(const char* name, const char* description, const char* defaultValue, const char* currentValue)
	{
		std::unique_lock lock(mutex_);
		PH_CVarParameter* param = InitCVar(name, description);
		if (!param) return nullptr;

		param->type = CVarType::STRING;

		GetCVarArray<PH_String>()->Add(defaultValue, currentValue, param);

		return param;
	}

	PH_CVarParameter* PH_CVarSystemImpl::InitCVar(const char* name, const char* description)
	{

		uint32_t namehash = StringUtilities::StringHash{ name };
		savedCVars[namehash] = PH_CVarParameter{};

		PH_CVarParameter& newParam = savedCVars[namehash];

		newParam.name = name;
		newParam.description = description;

		return &newParam;
	}

	AutoCVar_Float::AutoCVar_Float(const char* name, const char* description, double defaultValue, CVarFlags flags)
	{
		PH_CVarParameter* cvar = PH_CVarSystem::Get()->CreateFloatCVar(name, description, defaultValue, defaultValue);
		cvar->flags = flags;
		index = cvar->arrayIndex;
	}

	template<typename T>
	T GetCVarCurrentByIndex(int32_t index) {
		return PH_CVarSystemImpl::Get()->GetCVarArray<T>()->GetCurrent(index);
	}
	template<typename T>
	T* PtrGetCVarCurrentByIndex(int32_t index) {
		return PH_CVarSystemImpl::Get()->GetCVarArray<T>()->GetCurrentPtr(index);
	}


	template<typename T>
	void SetCVarCurrentByIndex(int32_t index, const T& data) {
		PH_CVarSystemImpl::Get()->GetCVarArray<T>()->SetCurrent(data, index);
	}


	double AutoCVar_Float::Get()
	{
		return GetCVarCurrentByIndex<CVarType>(index);
	}

	double* AutoCVar_Float::GetPtr()
	{
		return PtrGetCVarCurrentByIndex<CVarType>(index);
	}

	float AutoCVar_Float::GetFloat()
	{
		return static_cast<float>(Get());
	}

	float* AutoCVar_Float::GetFloatPtr()
	{
		float* result = reinterpret_cast<float*>(GetPtr());
		return result;
	}

	void AutoCVar_Float::Set(double f)
	{
		SetCVarCurrentByIndex<CVarType>(index, f);
	}

	AutoCVar_Int::AutoCVar_Int(const char* name, const char* description, int32_t defaultValue, CVarFlags flags)
	{
		PH_CVarParameter* cvar = PH_CVarSystem::Get()->CreateIntCVar(name, description, defaultValue, defaultValue);
		cvar->flags = flags;
		index = cvar->arrayIndex;
	}

	int32_t AutoCVar_Int::Get()
	{
		return GetCVarCurrentByIndex<CVarType>(index);
	}

	int32_t* AutoCVar_Int::GetPtr()
	{
		return PtrGetCVarCurrentByIndex<CVarType>(index);
	}

	void AutoCVar_Int::Set(int32_t val)
	{
		SetCVarCurrentByIndex<CVarType>(index, val);
	}

	void AutoCVar_Int::Toggle()
	{
		bool enabled = Get() != 0;

		Set(enabled ? 0 : 1);
	}

	AutoCVar_String::AutoCVar_String(const char* name, const char* description, const char* defaultValue, CVarFlags flags)
	{
		PH_CVarParameter* cvar = PH_CVarSystem::Get()->CreateStringCVar(name, description, defaultValue, defaultValue);
		cvar->flags = flags;
		index = cvar->arrayIndex;
	}

	const char* AutoCVar_String::Get()
	{
		return GetCVarCurrentByIndex<CVarType>(index).c_str();
	};

	void AutoCVar_String::Set(PH_String&& val)
	{
		SetCVarCurrentByIndex<CVarType>(index, val);
	}


	void PH_CVarSystemImpl::DrawImguiEditor()
	{
		static PH_String searchText = "";

		ImGui::InputText("Filter", &searchText);
		static bool bShowAdvanced = false;
		ImGui::Checkbox("Advanced", &bShowAdvanced);
		ImGui::Separator();
		cachedEditParameters.clear();

		auto addToEditList = [&](auto parameter)
		{
			bool bHidden = ((uint32_t)parameter->flags & (uint32_t)CVarFlags::Noedit);
			bool bIsAdvanced = ((uint32_t)parameter->flags & (uint32_t)CVarFlags::Advanced);

			if (!bHidden)
			{
				if (!(!bShowAdvanced && bIsAdvanced) && parameter->name.find(searchText) != PH_String::npos)
				{
					cachedEditParameters.push_back(parameter);
				};
			}
		};

		for (int i = 0; i < GetCVarArray<int32_t>()->lastCVar; i++)
		{
			addToEditList(GetCVarArray<int32_t>()->cvars[i].parameter);
		}
		for (int i = 0; i < GetCVarArray<double>()->lastCVar; i++)
		{
			addToEditList(GetCVarArray<double>()->cvars[i].parameter);
		}
		for (int i = 0; i < GetCVarArray<PH_String>()->lastCVar; i++)
		{
			addToEditList(GetCVarArray<PH_String>()->cvars[i].parameter);
		}

		if (cachedEditParameters.size() > 10)
		{
			std::unordered_map<PH_String, PH_DynamicArray<PH_CVarParameter*>> categorizedParams;

			//insert all the edit parameters into the hashmap by category
			for (auto p : cachedEditParameters)
			{
				int dotPos = -1;
				//find where the first dot is to categorize
				for (int i = 0; i < p->name.length(); i++)
				{
					if (p->name[i] == '.')
					{
						dotPos = i;
						break;
					}
				}
				PH_String category = "";
				if (dotPos != -1)
				{
					category = p->name.substr(0, dotPos);
				}

				auto it = categorizedParams.find(category);
				if (it == categorizedParams.end())
				{
					categorizedParams[category] = PH_DynamicArray<PH_CVarParameter*>();
					it = categorizedParams.find(category);
				}
				it->second.push_back(p);
			}

			for (auto [category, parameters] : categorizedParams)
			{
				//alphabetical sort
				std::sort(parameters.begin(), parameters.end(), [](PH_CVarParameter* A, PH_CVarParameter* B)
					{
						return A->name < B->name;
					});

				if (ImGui::BeginMenu(category.c_str()))
				{
					float maxTextWidth = 0;

					for (auto p : parameters)
					{
						maxTextWidth = std::max(maxTextWidth, ImGui::CalcTextSize(p->name.c_str()).x);
					}
					for (auto p : parameters)
					{
						EditParameter(p, maxTextWidth);
					}

					ImGui::EndMenu();
				}
			}
		}
		else
		{
			//alphabetical sort
			std::sort(cachedEditParameters.begin(), cachedEditParameters.end(), [](PH_CVarParameter* A, PH_CVarParameter* B)
				{
					return A->name < B->name;
				});
			float maxTextWidth = 0;
			for (auto p : cachedEditParameters)
			{
				maxTextWidth = std::max(maxTextWidth, ImGui::CalcTextSize(p->name.c_str()).x);
			}
			for (auto p : cachedEditParameters)
			{
				EditParameter(p, maxTextWidth);
			}
		}
	}
	void Label(const char* label, float textWidth)
	{
		constexpr float Slack = 50;
		constexpr float EditorWidth = 100;

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const ImVec2 lineStart = ImGui::GetCursorScreenPos();
		const ImGuiStyle& style = ImGui::GetStyle();
		float fullWidth = textWidth + Slack;

		ImVec2 textSize = ImGui::CalcTextSize(label);

		ImVec2 startPos = ImGui::GetCursorScreenPos();

		ImGui::Text(label);

		ImVec2 finalPos = { startPos.x + fullWidth, startPos.y };

		ImGui::SameLine();
		ImGui::SetCursorScreenPos(finalPos);

		ImGui::SetNextItemWidth(EditorWidth);
	}
	void PH_CVarSystemImpl::EditParameter(PH_CVarParameter* p, float textWidth)
	{
		const bool readonlyFlag = ((uint32_t)p->flags & (uint32_t)CVarFlags::EditReadOnly);
		const bool checkboxFlag = ((uint32_t)p->flags & (uint32_t)CVarFlags::EditCheckbox);
		const bool dragFlag = ((uint32_t)p->flags & (uint32_t)CVarFlags::EditFloatDrag);


		switch (p->type)
		{
		case CVarType::INT:

			if (readonlyFlag)
			{
				PH_String displayFormat = p->name + "= %i";
				ImGui::Text(displayFormat.c_str(), GetCVarArray<int32_t>()->GetCurrent(p->arrayIndex));
			}
			else
			{
				if (checkboxFlag)
				{
					bool bCheckbox = GetCVarArray<int32_t>()->GetCurrent(p->arrayIndex) != 0;
					Label(p->name.c_str(), textWidth);

					ImGui::PushID(p->name.c_str());

					if (ImGui::Checkbox("", &bCheckbox))
					{
						GetCVarArray<int32_t>()->SetCurrent(bCheckbox ? 1 : 0, p->arrayIndex);
					}
					ImGui::PopID();
				}
				else
				{
					Label(p->name.c_str(), textWidth);
					ImGui::PushID(p->name.c_str());
					ImGui::InputInt("", GetCVarArray<int32_t>()->GetCurrentPtr(p->arrayIndex));
					ImGui::PopID();
				}
			}
			break;

		case CVarType::FLOAT:

			if (readonlyFlag)
			{
				PH_String displayFormat = p->name + "= %f";
				ImGui::Text(displayFormat.c_str(), GetCVarArray<double>()->GetCurrent(p->arrayIndex));
			}
			else
			{
				Label(p->name.c_str(), textWidth);
				ImGui::PushID(p->name.c_str());
				if (dragFlag)
				{
					ImGui::InputDouble("", GetCVarArray<double>()->GetCurrentPtr(p->arrayIndex), 0, 0, "%.3f");
				}
				else
				{
					ImGui::InputDouble("", GetCVarArray<double>()->GetCurrentPtr(p->arrayIndex), 0, 0, "%.3f");
				}
				ImGui::PopID();
			}
			break;

		case CVarType::STRING:

			if (readonlyFlag)
			{
				PH_String displayFormat = p->name + "= %s";
				ImGui::PushID(p->name.c_str());
				ImGui::Text(displayFormat.c_str(), GetCVarArray<PH_String>()->GetCurrent(p->arrayIndex));

				ImGui::PopID();
			}
			else
			{
				Label(p->name.c_str(), textWidth);
				ImGui::InputText("", GetCVarArray<PH_String>()->GetCurrentPtr(p->arrayIndex));

				ImGui::PopID();
			}
			break;

		default:
			break;
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(p->description.c_str());
		}
	}

};