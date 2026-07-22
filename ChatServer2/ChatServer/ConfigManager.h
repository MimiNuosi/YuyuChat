#pragma once
#include "const.h"

struct SectionInfo
{
	SectionInfo() {};
	~SectionInfo() {
		_section_datas.clear();
	};

	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas;
	}

	SectionInfo& operator =(const SectionInfo& src) {
		if (&src == this) {
			return *this;
		}
		this->_section_datas = src._section_datas;
		return *this;
	}

	std::map<std::string, std::string> _section_datas;
	std::string operator [](const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return  "";
		}
		return _section_datas[key];
	}

	std::string GetValue(const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		// 这里可以添加一些边界检查  
		return _section_datas[key];
	}
};

class ConfigManager
{
public:
	~ConfigManager() {
		_config_map.clear();
	}

	ConfigManager(const ConfigManager& src) = delete;

	ConfigManager& operator =(const ConfigManager& src) = delete;

	SectionInfo operator [](const std::string& section) {
		if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo();
		}
		return _config_map[section];
	}

	static ConfigManager& Inst() {
		static ConfigManager cfg_mgr;
		return cfg_mgr;
	}

	std::string GetValue(const std::string& section, const std::string& key);
private:
	std::map<std::string, SectionInfo> _config_map;
	ConfigManager();
};

