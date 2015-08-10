#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>

using namespace boost::asio;
using ip::tcp;

class splice_helper_base 
{
public:
	virtual void start() = 0;
};

template<typename Session, typename StreamType1, typename StreamType2>
class splice_helper
	: public splice_helper_base
	, public boost::enable_shared_from_this<splice_helper<Session, StreamType1, StreamType2>>
{
	boost::shared_ptr<Session> session_;
	StreamType1 stream1_;
	StreamType2 stream2_;
	char b1_[1024];
	char b2_[1024];
public:
	splice_helper(boost::shared_ptr<Session> session, StreamType1 stream1, StreamType2 stream2)
		: session_(session)
		, stream1_(stream1)
		, stream2_(stream2)
	{
	}
	~splice_helper()
	{
		std::cout << "splice end" << std::endl;
	}

	void start()
	{
		start_splice(stream1_, stream2_, b1_);
		start_splice(stream2_, stream1_, b2_);
	}

	template<typename Type1, typename Type2>
	void start_splice(Type1 stream1, Type2 stream2, char* buf)
	{
		auto self = this->shared_from_this();
		stream1->async_read_some(buffer(buf, 1024), [this, self, stream1, stream2, buf](const boost::system::error_code& ec, std::size_t bytes){
			if(!ec)
			{
				async_write(*stream2, buffer(buf, bytes), [this, self, stream1, stream2, buf](const boost::system::error_code& ec, std::size_t bytes) {
					if(!ec)
					{
						start_splice(stream1, stream2, buf);
					}
				});
			}
		});
	}
};

template<typename Session, typename StreamType1, typename StreamType2>
boost::shared_ptr<splice_helper_base> make_splice(boost::shared_ptr<Session> session, StreamType1 stream1, StreamType2 stream2)
{
	auto sh = boost::make_shared<splice_helper<Session, StreamType1, StreamType2>>(session, stream1, stream2);
	return sh;
}

class ssl_session
	: public boost::enable_shared_from_this<ssl_session>
{
	io_service& io_;
	boost::shared_ptr<tcp::socket> socket_;
	ssl::context context_;
	ssl::stream<tcp::socket> ssl_stream_;
	const tcp::endpoint& endpoint_;
public:
	ssl_session(io_service& io, boost::shared_ptr<tcp::socket> socket, const tcp::endpoint& endpoint)
		: io_(io)
		, socket_(socket)
		, context_(ssl::context::sslv23)
		, ssl_stream_(io_, context_)
		, endpoint_(endpoint)
	{
	}

	void start()
	{
		auto self = shared_from_this();
		ssl_stream_.lowest_layer().async_connect(endpoint_, [this, self](const boost::system::error_code& ec){
			if(!ec)
			{
				ssl_stream_.async_handshake(ssl::stream_base::client, [this, self](const boost::system::error_code& ec) {
					if(!ec)
					{
						make_splice(shared_from_this(), &ssl_stream_, socket_)->start();
					}
					else
					{
						std::cout << "ssl handshake error:" << ec.message() << std::endl;
					}
				});
			}
			else
			{
				std::cout << "connect to https server error:" << ec.message() << std::endl;
			}
		});
	}
};

boost::shared_ptr<ssl_session> make_ssl_session(io_service& io, boost::shared_ptr<tcp::socket> socket, const tcp::endpoint& endpoint)
{
	return boost::make_shared<ssl_session>(io, socket, endpoint);
}

class ssl_server
{
	io_service& io_;
	tcp::acceptor acceptor_;
	tcp::endpoint endpoint_;
public:
	ssl_server(io_service& io)
		: io_(io)
		, acceptor_(io_, tcp::endpoint(tcp::v4(), 5000))
	{
		tcp::resolver resolver(io_);
		endpoint_ = *resolver.resolve(tcp::resolver::query("jp1.60in.com", "https"));
	}

	void start()
	{
		auto socket = boost::make_shared<tcp::socket>(io_);
		acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec){
			if(!ec)
			{
				on_connection(socket);	
				start();
			}
		});
	}

	void on_connection(boost::shared_ptr<tcp::socket> socket)
	{
		make_ssl_session(io_, socket, endpoint_)->start();
	}
};


int main()
{
	io_service io;

	ssl_server server(io);
	server.start();

	io.run();
}
