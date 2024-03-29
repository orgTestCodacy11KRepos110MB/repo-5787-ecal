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
#pragma once

#include <string>

#include "tui/viewmodel/table.hpp"

#include "model/monitor.hpp"
#include "model/data/service.hpp"

class ServicesViewModel : public TableViewModel<Service>
{
public:
  enum Column
  {
    Host, Process, Name, Heartbeat
  };

  ServicesViewModel(std::shared_ptr<MonitorModel> model_)
    : TableViewModel<Service>({ "Host", "Process", "Name", "Heartbeat" })
  {
    title = "Services";
    model_->AddModelUpdateCallback([this, model_] {
        TableViewModel::UpdateData(model_->Services());
    });
  }

  virtual std::string StringRepresentation(int column, const Service& value) override
  {
    switch (column)
    {
      case Column::Host:
        return value.host_name;
      case Column::Process:
        return value.unit_name;
      case Column::Name:
        return value.name;
      case Column::Heartbeat:
        return std::to_string(value.registration_clock);
      default: return "";
    }
  }
};
