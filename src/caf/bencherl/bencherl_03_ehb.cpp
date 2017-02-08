
/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2016                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENCE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include <iostream>
#include <thread> //sleep_for
#include <chrono>
#include <bitset>

#include "caf/all.hpp"

//using namespace std;
//using namespace caf;

//using ready_atom = atom_constant<atom("ready")>;
//using go_atom = atom_constant<atom("go")>;
//using done_atom = atom_constant<atom("done")>;

//#define CAF_ALLOW_UNSAFE_MESSAGE_TYPE(type_name)                               \
  //namespace caf {                                                              \
  //template <>                                                                  \
  //struct allowed_unsafe_message_type<type_name> : std::true_type {};           \
  //}

////-define(ACK, 100).
////-define(GSIZE, 20).
//enum {
  //ack = 100,
  //gsize = 20
//};
////-define(DATA, {a,b,c,d,e,f,g,h,i,j,k,l}). %% 104 bytes on a 64-bit machine
//struct data_t {
  //std::array<char,104> tmp; 
//};
//CAF_ALLOW_UNSAFE_MESSAGE_TYPE(data_t);

//template<class T>
//void foreach_receive_match(const vector<T>& vec, std::function<void(T&)> recv_fun) {
  //T match;
  //vector<bool> matches(vec.size(), false);
  //for (int i = 0; i < vec.size(); ++i) {
    //recv_fun(match);
    //auto it = std::find(match, begin(vec), end(vec));
    //if (it != vec.end()) {
      //size_t idx = std::distance(begin(vec), it);
      //matches[idx] = true;
    //}
  //}
  //if(!all_of(begin(matches), end(matches), [](bool x){return x;})) {
    //// wait for ever
    //this_thread::sleep_for(std::chrono::duration_values<chrono::hours>::max());
  //}
//}



////class pinger : public event_based_actor {
////public:
  ////pinger(actor_config& cfg) : event_based_actor(cfg) {
    ////this->set_default_handler(skip);
  ////}

  ////behavior_type make_behavior() override {
    //////return b_E_E_true_;
  ////}
////private:
////};

//void receiver(actor& gmaster, int senders_left) {

//}

//behavior sender(vector<actor> rs, int n) {
  //return {
  
  //};
//}

//actor group_fun(actor_system& system, actor& master, int loop) {
  //return system.spawn([=] (event_based_actor* self) {
    //auto gmaster = actor(self); 
		////Rs = lists:map(fun (_) ->
				////spawn_link(fun () -> receiver(GMaster, ?GSIZE) end)
				////end, lists:seq(1, ?GSIZE)),
    //vector<actor> rs; //????????????????? lots of copies (optimisation is required)
    //rs.reserve(gsize);
    //for (int i= 0; i < gsize; ++i) {
      //self->spawn(receiver, gmaster, gsize); 
    //}
		////Ss = lists:map(fun (_) ->
				////spawn_link(fun () ->
					////receive {GMaster, go} -> sender(Rs,Loop) end
				////end)
			////end, lists:seq(1, ?GSIZE)),
    //vector<actor> ss;
    //ss.reserve(gsize);
    //for (int i= 0; i < gsize; ++i) {
      //self->spawn([=](event_based_actor* self){
          //self->become({
            //[=] (go_atom) {
              //if (gmaster == self->current_sender()){
                //self->become(sender(rs, loop));
              //}
            //}
          //});
      //});
    //}
		////Master ! {self(), ready},
    //self->send(master, ready_atom::value);
		////receive {Master, go} -> ok end,
    //self->become({
      //[=] (go_atom) {
        //if (master == self->current_sender()){
					////lists:foreach(fun (S) -> S ! {GMaster, go} end, Ss),
          //for (auto& s: ss) {
            //self->send(s, go_atom::value);
          //}
					////lists:foreach(fun (R) -> receive {R, done} -> ok end end, Rs),
          //self->become({
            //[=] () {
            
            //}
          //});
        //}
      //}
    //});

  //});
//}

//void run(actor_system& system, int n, int m) {
  //scoped_actor self{system};
  //auto master = actor(self);
	////Gs = lists:map(fun (_) -> group(Master, M) end, lists:seq(1, N)),
  //vector<actor> gs;
  //gs.reserve(n);
  //for (int i = 0; i < n; ++i) {
    //gs.emplace_back(group_fun(system, master, m));
  //}
	////lists:foreach(fun (G) -> receive {G, ready} -> ok end end, Gs),
  //foreach_receive_match<actor>(gs, [&](actor& res){
    //self->receive([&] (ready_atom) {
      //res = actor_cast<actor>(self->current_sender());
    //});
  //});
	////lists:foreach(fun (G) -> G ! {Master, go} end, Gs),
  //for (auto& g: gs) {
    //self->send(g, master, go_atom::value);
  //}
	////lists:foreach(fun (G) -> receive {G, done} -> ok end end, Gs),
  //foreach_receive_match<actor>(gs, [&](actor& res){
    //self->receive([&](done_atom){
      //res = actor_cast<actor>(self->current_sender());
    //});
  //});
//}

//void usage() {
  //cout << "usage: bencherl_03_ehb VERSION NUM_CORES" << endl
       //<< "       VERSION:      short|intermediate|long " << endl
       //<< "       NUM_CORES:    number of cores" << endl << endl
       //<< "  for details see http://release.softlab.ntua.gr/bencherl/" << endl;
  //exit(1);
//}

int main(int argc, char** argv) {
  // configuration
  //if (argc != 3)
    //usage();
  //string version = argv[1];
  //int f1;
  //int f2;
  //if (version == "test") {
    //f1 = 1;
    //f2 = 1;
  //} else if (version == "short") {
    //f1 = 1;
    //f2 = 4;
  //} else if (version == "intermediate") {
    //f1 = 2;
    //f2 = 8;
  //} else if (version == "long") {
    //f1 = 8;
    //f2 = 8;
  //} else {
    //std::cerr << "version musst be short, intermediate or long" << std::endl;
    //exit(1);
  //}
  //int cores = std::stoi(argv[2]);
  //actor_system_config cfg;
  //cfg.parse(argc, argv, "caf-application.ini");
  //actor_system system{cfg};
  //int n = f1 * cores;
  //int m = f2 * cores;
  //run(system, n, m); 
}

