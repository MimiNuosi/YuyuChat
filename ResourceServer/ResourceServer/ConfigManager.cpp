#include "ConfigManager.h"
#include <algorithm>

// 辅助清洗函数：剔除首尾 \r, \n, 空格和制表符
static void TrimString(std::string& s) {
    if (s.empty()) return;
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
}

std::string ConfigManager::GetValue(const std::string& section, const std::string& key) {
    if (_config_map.find(section) == _config_map.end()) {
        return "";
    }
    return _config_map[section].GetValue(key);
}

boost::filesystem::path ConfigManager::GetFileOutPath()
{
    return _static_path;
}

void ConfigManager::InitPath()
{
    // 获取当前工作目录  
    boost::filesystem::path current_path = boost::filesystem::current_path();
    std::string bindir = _config_map["Output"].GetValue("Path");
    std::string staticdir = _config_map["Static"].GetValue("Path");

    // 兜底去除多余换行符与空格
    TrimString(bindir);
    TrimString(staticdir);

    if (bindir.empty()) bindir = "bin";
    if (staticdir.empty()) staticdir = "static";

    _bin_path = current_path / bindir;
    _static_path = _bin_path / staticdir;

    boost::system::error_code ec;
    // 检查路径是否存在并安全创建（带 ec 参数避免异常崩溃）
    if (!boost::filesystem::exists(_static_path, ec)) {
        if (boost::filesystem::create_directories(_static_path, ec)) {
            std::cout << "路径已成功创建: " << _static_path.string() << std::endl;
        }
        else {
            std::cerr << "创建路径失败: " << _static_path.string()
                << " (Error: " << ec.message() << ")" << std::endl;
        }
    }
    else {
        std::cout << "路径已存在: " << _static_path.string() << std::endl;
    }
}

ConfigManager::ConfigManager()
{
    boost::filesystem::path config_path = boost::filesystem::current_path() / "config.ini";
    std::cout << "config path is " << config_path.string() << "\n";

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(config_path.string(), pt);

    for (const auto& section_pair : pt) {
        std::string section_name = section_pair.first;
        TrimString(section_name);

        const boost::property_tree::ptree& section_tree = section_pair.second;
        std::map<std::string, std::string> section_config;
        for (const auto& key_value_pair : section_tree) {
            std::string key = key_value_pair.first;
            std::string value = key_value_pair.second.get_value<std::string>();

            // 彻底清除 Windows INI 文件中的 \r
            TrimString(key);
            TrimString(value);
            section_config[key] = value;
        }

        SectionInfo sectionInfo;
        sectionInfo._section_datas = std::move(section_config);
        _config_map[section_name] = sectionInfo;
    }

    // 构造完成后立即初始化目录
    InitPath();

    for (const auto& section_entry : _config_map) {
        const std::string& section_name = section_entry.first;
        SectionInfo section_config = section_entry.second;
        std::cout << "[" << section_name << "]" << std::endl;
        for (const auto& key_value_pair : section_config._section_datas) {
            std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl;
        }
    }
}
