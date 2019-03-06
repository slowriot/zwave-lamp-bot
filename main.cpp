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

  lamps.emplace(4, "Dining Table");
  lamps.emplace(3, "Armchairs");
  std::initializer_list constexpr const power_values{0, 5, 10, 15, 25, 50, 75, 99};

  auto send_lamp_list = [&](int_fast64_t chat_id, std::string const &reply){
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

  listener.set_callback_message_json([&](nlohmann::json const &input){
    auto const &message(telegram::types::message::from_json(input));

    if(message.text && *message.text == "/start") {                             // if they've asked us to start, send them a keyboard
      //sender.send_message(message.chat.id, "Hello, " + (message.from ? message.from->first_name : "visitor") + "!  What can I illuminate for you today?");
      send_lamp_list(message.chat.id, "Hello, " + (message.from ? message.from->first_name : "visitor") + "!  What can I illuminate for you today?");
    }
  });

  listener.set_callback_query_callback([&](telegram::types::callback_query const &query){
    /// Set up a listener for callback queries
    std::cout << "DEBUG: callback query callback firing " << *query.data << std::endl;
    if(!query.data) {
      sender.send_message(query.from.id, "Something went wrong - no query data.");
      return;
    }

    if(*query.data == "noop") {
      sender.send_message(query.from.id, "You boop the button, but it does nothing.");
      return;
    }
    std::cout << "DEBUG: lamp id [" << query.data->substr(0, query.data->find('_')) << "]" << std::endl;
    std::cout << "DEBUG: new value [" << query.data->substr(query.data->find('_'), std::string::npos) << "]" << std::endl;

    auto &this_lamp{lamps.at(std::stoi(query.data->substr(0, query.data->find('_'))))};
    this_lamp.value = std::stoi(query.data->substr(query.data->find('_') + 1, std::string::npos));
    send_lamp_list(query.from.id, "Setting " + this_lamp.name + " to " + std::to_string(this_lamp.value) + ".");
  });

  listener.set_num_threads(1);                                                  // run single-threaded as we don't want concurrent modifications
  listener.run();                                                               // launch the listener - this call blocks until the listener is terminated
  return EXIT_SUCCESS;
};
