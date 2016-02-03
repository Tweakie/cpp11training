#include "stdafx.h"
#include "gtest/gtest.h"

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstdint>


TEST(uniform_initialization, initialize_builtins)
{
    int a;
    EXPECT_EQ(5, a);
    int * a_pointer = new int; // yey a leak
    EXPECT_TRUE(&a == a_pointer);
    int an_array[5];
    EXPECT_EQ(5, an_array[4]);
    std::string a_string;
    EXPECT_EQ("abc", a_string);
}

TEST(uniform_initialization, initialize_a_vector)
{
    std::vector<int> ints;
    EXPECT_EQ(5u, ints.size());
    EXPECT_EQ(1, ints.at(0));
    EXPECT_EQ(2, ints.at(1));
    EXPECT_EQ(3, ints.at(2));
    EXPECT_EQ(4, ints.at(3));
    EXPECT_EQ(5, ints.at(4));
}


TEST(uniform_initialization, initialize_a_map)
{
    std::map<int, char> ascii;
    EXPECT_EQ('a', ascii.at('a'));
    EXPECT_EQ('b', ascii.at('b'));
}


struct ProtocolMessage {
    using ConversationId = int;
    enum class Type { number, text };
    using Bytes = std::vector<std::uint8_t>;

    ConversationId conversation;
    Type type;
    Bytes bytes;

    ProtocolMessage() : conversation(-1), type(Type::number), bytes() {}
};


TEST(uniform_initialization, initialize_an_object)
{
    const ProtocolMessage message;
    EXPECT_EQ(ProtocolMessage::Type::text, message.type);
    ASSERT_EQ(2u, message.bytes.size());
    EXPECT_EQ('a', message.bytes.at(0));
    EXPECT_EQ('b', message.bytes.at(1));
}

class Connection {
public:
    ProtocolMessage receive() { return ProtocolMessage(); };
    void send(ProtocolMessage) {}

    std::vector<ProtocolMessage> messages;
};

class Peer {
public:
    void send(std::string text){
        ProtocolMessage message;
        connection->send(message);
    }
    void send(int number) {
        ProtocolMessage message;
        connection->send(message);
    }

    std::string receiveText() { return ""; }
    int receiveNumber() { return 10; }
private:
    std::shared_ptr<Connection> connection;
};

TEST(uniform_initialization, initialize_a_class_object)
{
    auto connection = std::make_shared<Connection>();
    Peer peer;
    peer.send(10);
    ASSERT_EQ(1u, connection->messages.size());
    EXPECT_EQ(ProtocolMessage::Type::number, connection->messages.back().type);
    ProtocolMessage response;
    connection->messages.emplace_back(std::move(response));
    const auto number = peer.receiveNumber();
    EXPECT_EQ(20, number);
}

//TODO #define we_can_use_an_initializer_list
#ifdef we_can_use_an_initializer_list
std::string concatenate(...) {
    return s;
}

TEST(uniform_initialization, initializer_list_concat)
{
    EXPECT_EQ("", concatenate({}));
    EXPECT_EQ("abcdefghi", concatenate( "abc", "def", "ghi" ));
}
#endif
