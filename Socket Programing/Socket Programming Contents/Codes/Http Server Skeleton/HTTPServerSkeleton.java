package httpserverskeleton;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;

public class HTTPServerSkeleton {
    static final int PORT = 6789;
    
    public static void main(String[] args) throws IOException {
        
        ServerSocket serverConnect = new ServerSocket(PORT);
        System.out.println("Server started.\nListening for connections on port : " + PORT + " ...\n");
        while(true)
        {
            Socket s=serverConnect.accept();
            BufferedReader in = new BufferedReader(new InputStreamReader(s.getInputStream()));
            PrintWriter pr = new PrintWriter(s.getOutputStream());
            String input = in.readLine();
            System.out.println("Here Input : "+input);
        }
        
    }
    
}
