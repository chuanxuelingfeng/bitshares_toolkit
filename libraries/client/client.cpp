#include <bts/client/client.hpp>
#include <bts/client/messages.hpp>
#include <bts/net/node.hpp>
#include <bts/blockchain/chain_database.hpp>
#include <fc/reflect/variant.hpp>

namespace bts { namespace client {

    namespace detail 
    { 
       class client_impl : public bts::net::node_delegate
       {
          public:
            virtual bool has_item( const net::item_id& id ) override
            {
               return false;
            }
            virtual void     sync_status( uint32_t item_type, uint32_t item_count ) override {};
            
            /**
             *  Call any time the number of connected peers changes.
             */
            virtual void     connection_count_changed( uint32_t c ) override {};
            /**
             *  @brief allows the application to validate a message prior to 
             *         broadcasting to peers.
             */
            virtual void handle_message( const bts::net::message& msg ) override
            {
                 switch( msg.msg_type )
                 {
                     case trx_message_type:
                        validate_trx_message( msg.as<trx_message>() );
                        break;
                     case block_message_type:
                        validate_block_message( msg.as<block_message>() );
                        break;
                     case signature_message_type:
                        validate_signature_message( msg.as<signature_message>() );
                        break;
                 }
            } // validate_broadcast

            /**
             *  Assuming all data elements are ordered in some way, this method should
             *  return up to limit ids that occur *after* from_id.
             */
            virtual std::vector<net::item_hash_t> get_item_ids( const net::item_id& from_id, uint32_t& remaining_to_get, uint32_t limit ) override 
            {
                FC_ASSERT( _chain_db != nullptr );

                if( limit > 2000 ) 
                    limit = 2000;

                std::vector<net::item_hash_t> items; 
                items.reserve( limit );

                auto block_num = _chain_db->fetch_block_num( from_id.item_hash );
                for( auto i = block_num + 1; i < limit; ++i )
                {
                   items.push_back( _chain_db->fetch_block( i ).id() );
                }

                // TODO: set remaining_to_get to the number of blocks after items.back(), or 0 if there are none

                return items;
            } // get_headers


            /**
             *  Given the hash of the requested data, fetch the body. 
             */
            virtual bts::net::message get_item( const net::item_id& id ) override
            {
                FC_ASSERT( _chain_db != nullptr );

                auto block_num = _chain_db->fetch_block_num( id.item_hash );
                auto blk       = _chain_db->fetch_trx_block( block_num );
                auto sig       = _chain_db->fetch_block_signature( id.item_hash );

                return bts::net::message( block_message( id.item_hash, blk, sig ) );
            }


            virtual void handle_body( const bts::net::message& msg )
            {
                FC_ASSERT( msg.msg_type == block_message::type );
                // attempt to push the block.
            }

            virtual void validate_trx_message( const trx_message& msg )
            {
            }
            virtual void validate_block_message( const block_message& msg )
            {
            }
            virtual void validate_signature_message( const signature_message& msg )
            {
            }
            
            bts::blockchain::chain_database_ptr  _chain_db;
            bts::wallet::wallet_ptr              _wallet;
       };
    }

    client::client()
    :my( new detail::client_impl() )
    {
    }

    client::~client(){}

    void client::set_chain( const bts::blockchain::chain_database_ptr& ptr )
    {
       my->_chain_db = ptr;
    }

    void client::set_wallet( const bts::wallet::wallet_ptr& wall )
    {
       my->_wallet = wall;
    }


} } // bts::client