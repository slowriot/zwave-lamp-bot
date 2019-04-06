#define TELEGRAM_NO_LISTENER_FCGI
#include <libtelegram/libtelegram.h>

auto main()->int {
  std::ifstream token_file("token.txt");
  std::string token((std::istreambuf_iterator<char>(token_file)),
                     std::istreambuf_iterator<char>());
  token.erase(std::remove_if(token.begin(), token.end(), isspace), token.end());

  telegram::sender sender(token);                                               // create a sender with our token for outgoing messages
  telegram::listener::poll listener(sender);                                    // create a polling listener which will process incoming requests, polling using the sender

  struct lamp_type {
    std::string name;
    unsigned int value = 0;

    lamp_type(std::string const &new_name)
      : name{new_name} {
      /// Constructor
    }
  };
  std::map<unsigned int, lamp_type> lamps;

  lamps.emplace(2, "Corner Lamp");
  lamps.emplace(3, "Dining Table");
  lamps.emplace(4, "Armchairs");
  lamps.emplace(5, "Bedroom Lamp");
  std::initializer_list constexpr const power_values{0, 10, 15, 25, 40, 60, 75, 99};
  urdl::url const zwave_api_endpoint{"http://127.0.0.1:8090/valuepost.html"};

  auto send_lamp_list = [&](int_fast64_t chat_id, std::string const &reply){
    /// Send the keyboard with the list of lamps and their settings
    telegram::types::reply_markup::inline_keyboard_markup reply_markup;
    for(auto const &lamp : lamps) {
      reply_markup.keyboard_buttons.emplace_back();
      reply_markup.keyboard_buttons.back().emplace_back(lamp.second.name, "noop");
      reply_markup.keyboard_buttons.emplace_back();
      for(unsigned int value : power_values) {
        std::string label;
        if(value == lamp.second.value) {
          label = "[" + std::to_string(value) + "]";
        } else {
          label = std::to_string(value);
        }
        reply_markup.keyboard_buttons.back().emplace_back(label, std::to_string(lamp.first) + "_" + std::to_string(value));
      }
    }
    sender.send_message(chat_id,
                        reply,
                        reply_markup);
  };

  auto set_lamp_level = [&](unsigned int lamp_id, unsigned int new_level) {
    /// Update a z-wave device's level
    httplib::Client http_client(zwave_api_endpoint.host().c_str(), zwave_api_endpoint.port(), 10);
    auto http_result{http_client.Post((zwave_api_endpoint.path() + "?" + zwave_api_endpoint.query()).c_str(), // endpoint & query
                                      (std::to_string(lamp_id) + "-SWITCH MULTILEVEL-user-byte-1-0=" + std::to_string(new_level)).c_str(), // post data
                                      "application/x-www-form-urlencoded")};    // content type
    if(!http_result) {
      throw std::runtime_error("Error posting to ZWave URL " + zwave_api_endpoint.to_string());
    } else if (http_result->status != 200) {
      throw std::runtime_error("Error posting to ZWave URL " + zwave_api_endpoint.to_string() + ": " + std::to_string(http_result->status));
    }
    if(http_result->body.empty()) {
      throw std::runtime_error("Empty return when posting to ZWave URL " + zwave_api_endpoint.to_string() + ": " + std::to_string(http_result->status));
    }
  };

  listener.set_callback_message_json([&](nlohmann::json const &input){
    /// Set up a listener for messages
    auto const &message(telegram::types::message::from_json(input));

    if(message.text && *message.text == "/start") {                             // if they've asked us to start, send them a keyboard
      send_lamp_list(message.chat.id, "Hello, " + (message.from ? message.from->first_name : "visitor") + "!  What can I illuminate for you today?");
    }
  });

  listener.set_callback_query_callback([&](telegram::types::callback_query const &query){
    /// Set up a listener for callback queries
    if(!query.data) {
      sender.send_message(query.from.id, "Something went wrong - no query data.");
      return;
    }

    if(*query.data == "noop") {
      sender.send_message(query.from.id, "You boop the button, but it does nothing.");
      return;
    }
    unsigned int const lamp_id{static_cast<unsigned int>(std::stoi(query.data->substr(0, query.data->find('_'))))};
    auto &this_lamp{lamps.at(lamp_id)};
    this_lamp.value = std::stoi(query.data->substr(query.data->find('_') + 1u, std::string::npos));
    std::cout << "Request from [" << query.from.first_name;
    if(query.from.last_name) std::cout << " " << *query.from.last_name;
    if(query.from.username) std::cout << " (" << *query.from.username << ")";
    std::cout << "], lamp id [" << lamp_id << "], new value [" << this_lamp.value << "]" << std::endl;

    send_lamp_list(query.from.id, "Setting " + this_lamp.name + " to " + std::to_string(this_lamp.value) + ".");
    set_lamp_level(lamp_id, this_lamp.value);
  });

  listener.set_num_threads(1);                                                  // run single-threaded as we don't want concurrent modifications
  listener.run();                                                               // launch the listener - this call blocks until the listener is terminated
  return EXIT_SUCCESS;
};
