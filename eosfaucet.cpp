#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;

#define EOS_SYMBOL symbol("EOS",4)

CONTRACT eosfaucet : public contract {
public:

    eosfaucet(name receiver, name code, datastream<const char *> ds)
            : contract(receiver, code, ds), _limit_table(_code, _code.value) {}

    ACTION get(name user) {
        auto iterator = _limit_table.find(user.value);

        if (iterator != _limit_table.end()) {
            //用户在表里
            auto find = _limit_table.get(user.value);

            //判断等待时间
            eosio_assert(find.time < now(), "you can not get EOS yet");

            _limit_table.modify(iterator, get_self(), [&](auto &row) {
                row.time = now() + 60;
                sendEOS(user);
            });

        } else {
            //用户没在表里
            _limit_table.emplace(get_self(), [&](auto &row) {
                row.name = user;
                row.time = now() + 60;

                //给用户发送EOS
                sendEOS(user);
            });
        }
    }

private:

    void sendEOS(name user) {
        asset money = asset(10, EOS_SYMBOL);
        action(
                permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), user, money, std::string(""))
        ).send();
    }

    TABLE limit {
        name name;
        uint32_t time = 0;

        uint64_t primary_key() const { return name.value; }
    };

    typedef eosio::multi_index<"limit"_n, limit> limit_table;
    limit_table _limit_table;

};

EOSIO_DISPATCH(eosfaucet, (get))