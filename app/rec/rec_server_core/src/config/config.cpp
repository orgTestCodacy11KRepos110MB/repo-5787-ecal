/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2019 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

#include "config.h"

#include <tinyxml2.h>
#include <stdio.h>

#include <ecal_utils/str_convert.h>

#include <rec_client_core/ecal_rec_logger.h>

#include "config_v1.h"
#include "config_v2to4.h"


namespace eCAL
{
  namespace rec_server
  {
    namespace config
    {
      bool applyConfig(const eCAL::rec_server::RecServerConfig& config, eCAL::rec_server::RecServerImpl& rec_server)
      {
        // Set settings that can fail
        if (!rec_server.SetEnabledRecClients            (config.enabled_clients_config_)
           || !rec_server.SetUsingBuiltInRecorderEnabled(config.built_in_recorder_enabled_)
           || !rec_server.SetRecordMode                 (config.record_mode_, config.listed_topics_))
        {
          return false;
        }

        // Set settings that cannot fail
        rec_server.SetMeasRootDir             (config.root_dir_);
        rec_server.SetMeasName                (config.meas_name_);
        rec_server.SetMaxFileSizeMib          (config.max_file_size_);
        rec_server.SetDescription             (config.description_);
        rec_server.SetOneFilePerTopicEnabled  (config.one_file_per_topic_);
        rec_server.SetPreBufferingEnabled     (config.pre_buffer_enabled_);
        rec_server.SetMaxPreBufferLength      (config.pre_buffer_length_);
        rec_server.SetUploadConfig            (config.upload_config_);

        return true;
      }

      bool writeConfigToFile(const eCAL::rec_server::RecServerImpl& rec_server, const std::string& path)
      {
        return config_v2to4::writeConfigFile(rec_server, path);
      }

      bool readConfigFromFile(eCAL::rec_server::RecServerImpl& rec_server, const std::string& path, int* version)
      {
        FILE* xml_file;
#ifdef WIN32
        std::wstring w_path = EcalUtils::StrConvert::Utf8ToWide(path);
        xml_file = _wfopen(w_path.c_str(), L"rb");
#else
        xml_file = fopen(path.c_str(), "rb");
#endif // WIN32

        if (xml_file == nullptr)
        {
          eCAL::rec::EcalRecLogger::Instance()->error(std::string("Error opening file \"") + path + "\"");
          return false;
        }

        tinyxml2::XMLDocument document;
        tinyxml2::XMLError errorcode = document.LoadFile(xml_file);

        fclose(xml_file);

        if (errorcode != tinyxml2::XML_SUCCESS)
        {
#if TINYXML2_MAJOR_VERSION >= 6
          eCAL::rec::EcalRecLogger::Instance()->error(std::string("Error loading file from \"") + path + "\": " + document.ErrorStr());
#else // TINYXML2_MAJOR_VERSION
            eCAL::rec::EcalRecLogger::Instance()->error(std::string("Error loading file from \"") + path + "\": " + document.GetErrorStr1());
#endif // TINYXML2_MAJOR_VERSION
          return false;
        }

        // For Upwards and Backwards compatibility we support multiple configs in one file.
        std::map<int, tinyxml2::XMLElement*> config_elements;

        for(auto main_config_element = document.FirstChildElement(); main_config_element != nullptr; main_config_element = main_config_element->NextSiblingElement())
        {
          if (std::string(main_config_element->Name()) == config_v2to4::ELEMENT_NAME_MAIN_CONFIG)
          {
            if (main_config_element->Attribute(config_v2to4::ATTRIBUTE_NAME_MAIN_CONFIG_VERSION) != nullptr)
            {
              std::string config_version_number_string = main_config_element->Attribute(config_v2to4::ATTRIBUTE_NAME_MAIN_CONFIG_VERSION);
              int config_version_number = 0;
              try
              {
                config_version_number = std::stoi(config_version_number_string);
              }
              catch (const std::exception&)
              {}

              config_elements[config_version_number] = main_config_element;
            }
            else
            {
              config_elements[0] = main_config_element;
            }
          }
          else if (std::string(main_config_element->Name()) == config_v1::ELEMENT_NAME_MAIN_CONFIG)
          {
            config_elements[1] = main_config_element;
          }
        }

        // Read a v2 and updwarts config, starting with the latest native version.
        for (int v = NATIVE_CONFIG_VERSION; v >= 2; v--)
        {
          if (config_elements.find(v) != config_elements.end())
          {
            // Load config
            bool success = applyConfig(config_v2to4::readConfig(config_elements[v]), rec_server);
            if (success && (version != nullptr))
            {
              (*version) = v;
            }
            return success;
          }
        }

        {
          // Load some other config, although the version is not known
          for (const auto& version_config_pair : config_elements)
          {
            if (version_config_pair.first > NATIVE_CONFIG_VERSION)
            {
              bool success = applyConfig(config_v2to4::readConfig(version_config_pair.second), rec_server);
              if (success && (version != nullptr))
              {
                (*version) = version_config_pair.first;
              }
              return success;
            }
          }

          // Load an un-versioned config
          if (config_elements.find(0) != config_elements.end())
          {
            bool success = applyConfig(config_v2to4::readConfig(config_elements[0]), rec_server);
            if (success && (version != nullptr))
            {
              (*version) = 0;
            }
            return success;
          }
        }

        eCAL::rec::EcalRecLogger::Instance()->error(std::string("Error Loading file from \"") + path + "\": The file is not a valid eCAL Rec config.");
        return false;
      }
    }
  }
}

