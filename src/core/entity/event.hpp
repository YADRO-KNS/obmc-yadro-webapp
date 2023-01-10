// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2023, KNS Group LLC (YADRO)

#pragma once

#include <core/entity/entity_interface.hpp>

#include <functional>
#include <vector>

namespace app
{

template <typename TEnumEvent>
class Event : public virtual IEvent<TEnumEvent>
{
    using Subscriber = typename IEvent<TEnumEvent>::Subscriber;

  public:
    ~Event() override = default;

    /**
     * @brief Add subscriber to observe query process events.
     *
     * @note thread unsafe
     *
     * @param[in] subscriber - the subscriber
     */
    void addEventSubscriber(const Subscriber&& subscriber) override
    {
        subscribers.emplace_back(subscriber);
    }

  protected:
    /**
     * @brief Sends a signal to subscribers with specify TEnumEvent.
     *
     * @note thread unsafe
     *
     * @param[in] event - the type of event
     */
    void emitEvent(TEnumEvent event) const override
    {
        auto callback = [event](const Subscriber& callback) {
            std::invoke(callback, event);
        };
        std::for_each(subscribers.begin(), subscribers.end(), callback);
    }

  private:
    std::vector<Subscriber> subscribers;
};

} // namespace app
