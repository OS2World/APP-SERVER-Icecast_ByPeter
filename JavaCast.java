import java.net.*;
import java.lang.*;
import java.io.*;

public class JavaCast extends Thread {

	JavaCast() {
		start();
	}

	public void run() {
		try {
			System.out.println("Starting up the server...");

			ThreadGroup threadGroup = new ThreadGroup("clients");
			
			ServerSocket ssEncoder, ssClient;
			
			ssEncoder = new ServerSocket(8001);
			ssClient = new ServerSocket(8000);

			EncoderThread et = new EncoderThread(ssEncoder.accept(), threadGroup);
			System.out.println("Encoder connected...");
			while (true) {
				Socket client = ssClient.accept();

				System.out.println("Client connected...");

				ClientThread ct = new ClientThread(client, threadGroup);
			}	
		} catch (Exception e) {
			System.out.println(e.toString());
		}
	}

	public static void main(String argv[]) {
		new JavaCast();
	}

}

class EncoderThread extends Thread {

	Socket sock;
	ThreadGroup threadGroup;
        Thread[] threadList = new Thread[25];
	byte buffer[] = new byte[1024];

	EncoderThread(Socket s, ThreadGroup ts) {
		sock = s;
		threadGroup = ts;
		start();
	}

	public void run() {
		try {
			DataInputStream in = new DataInputStream(sock.getInputStream());
			BufferedWriter out = new BufferedWriter(new OutputStreamWriter(sock.getOutputStream()));

			while (in.readUnsignedByte() != 10) ;
			out.write("OK\n");
			out.flush();
			
			while (true) {
				in.readFully(buffer);
					
				threadGroup.enumerate(threadList);

				for (int i = 0; i < threadList.length; i++) {
 					if (threadList[i] != null && threadList[i].isAlive()) {
						((ClientThread)(threadList[i])).sendData(new String(buffer));
					}
				}
			}
		} catch (Exception e) {
			System.out.println(e.toString());
		}
	}
}

class ClientThread extends Thread {
	Socket sock;
	boolean dataAvail;
	String buffer;

	ClientThread(Socket s, ThreadGroup tg) {
		super(tg, "client");

		sock = s;
		start();
	}

	public void run() {
		serveClient();	
	}

	public synchronized void serveClient() {
		try {
			BufferedWriter out = new BufferedWriter(new OutputStreamWriter(sock.getOutputStream()));

			out.write("HTTP/1.0 200 OK\n");
			out.write("Content-type: audio/x-mpeg\n");
			out.write("\n");

			while (true) {
				wait();
				out.write(buffer);
			}
	        } catch (Exception e) {
 			System.out.println(e.toString());
		}	
	}

	public synchronized void sendData(String s) {
		buffer = s;
		notify();
	}
}
