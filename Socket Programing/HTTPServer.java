
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.file.Files;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

public class HTTPServer  {


    public static void main(String[] args) throws IOException {

        ServerSocket ss = new ServerSocket(Integer.parseInt(Config.getProperty("PORT")));
        PrintWriter pr = null;
        try{
            pr = new PrintWriter(new BufferedWriter(new FileWriter("Log.txt",true)),true);
        }catch (IOException ex){
            ex.printStackTrace();
        }

        while (true){
            Socket socket = ss.accept();
            new Thread(new RequestProcess(socket,pr)).start();

        }
        //pr.close();
    }


}

class Config{
    static Properties prop = new Properties();
    static
    {
        try(FileReader fr = new FileReader("rsrc.properties")){
            prop.load(fr);

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    static String getProperty(String key){
        return prop.getProperty(key);
    }
}

class RequestProcess implements Runnable {

    Socket socket;
    BufferedReader br;
    OutputStream os;
    PrintWriter pr;
    DataOutputStream dos;
    String method, fileName, httpVersion,userName,requestLine,mimeType = "text/html";
    long fileSize;
    Map<String, String> mimeTypeMap = new HashMap<>();
    StringBuilder sb = new StringBuilder();

    public RequestProcess(Socket socket,PrintWriter pr) {
        this.socket = socket;

        try {
            br = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            os = socket.getOutputStream();
            this.pr = pr;
            dos = new DataOutputStream(socket.getOutputStream());
        } catch (IOException e) {
            e.printStackTrace();
        }
        try {

            char[] chars = new char[1000];

            br.read(chars);

            sb.append(new String(chars).trim());


        }catch (IOException ex){
            ex.printStackTrace();
        }


//        mimeTypeMap.put(".jpg","image/jpg");
//        mimeTypeMap.put(".png","image/png");
//        mimeTypeMap.put(".html","text/html");
//        mimeTypeMap.put(".pdf","application/pdf");
//        mimeTypeMap.put(".bmp","image/bmp");
//        mimeTypeMap.put(".ico","image/x-icon");
//        mimeTypeMap.put(".gif","image/gif");
//        mimeTypeMap.put(".jpeg","image/jpeg");
//        mimeTypeMap.put(".txt","text/plain");
//        mimeTypeMap.put(".mp4","video/mp4");

    }

    public void MsgExtract(){
        String[] strArray = sb.toString().split("\\n");
        requestLine = strArray[0];

        String[] temp = requestLine.split(" ");

        method = temp[0].toUpperCase();
        fileName = temp[1];
        if(fileName.equals("/")) fileName = "/index.html";
        httpVersion = temp[2];
        if(method.equalsIgnoreCase("post")){
            String postLine = strArray[strArray.length-1];
            userName = postLine.split("=")[1];

        }
    }

    void executeMethod(){
        String statusCode = "";
        switch (method){
            case "GET":

                File file = new File(Config.getProperty("HTML_FILE_LOCATION")+fileName);
                if(file.exists()){
                    fileSize = file.length();
                    sendResponse("HTTP_SUCCES",file);
                    statusCode = "200";
                }else{
                    file = new File(Config.getProperty("HTML_FILE_LOCATION")+"/not_found.png");
                    fileSize = file.length();
                    sendResponse("HTTP_FILE_NOT_FOUND",file);
                    statusCode = "404";
                }
                break;
            case "POST":
                File htmlFile = new File(Config.getProperty("HTML_FILE_LOCATION")+"/index.html");
                FileInputStream fi = null;
                try {
                    fi = new FileInputStream(htmlFile);
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                }
                byte[] data = new byte[(int) htmlFile.length()];
                try {
                    fi.read(data);
                    fi.close();
                    String str = new String(data);
                    str = str.replaceAll("Post->","Post-> "+userName);
                    fileSize = str.length();
                    sendHeader("HTTP_SUCCES");
                    dos.write(str.getBytes());
                    dos.flush();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                statusCode = "200";
                break;


        }

        String log = socket.getInetAddress().getHostAddress() + "--" +"["+ new Date().toString() + "] \"" + requestLine.trim() +"\" " + statusCode + " " + fileSize;
        pr.println(log);

        try {
            socket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void printFullMessage() {
        System.out.println(sb);
    }

    void sendHeader(String status){
        StringBuilder sb = new StringBuilder();

        sb.append(Config.getProperty(status)+"\r\n");
        sb.append((new Date()).toString()+"\r\n");
        sb.append("Server: localhost\r\n");
        sb.append("Content-Length: "+fileSize + "\r\n");
        sb.append("Content-Type: "+ mimeType);
        sb.append("\r\n\n");

        try {
            dos.write(sb.toString().getBytes());
            dos.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    void sendResponse(String status,File file){
        try {
            mimeType = Files.probeContentType(file.toPath());
        } catch (IOException e) {
            e.printStackTrace();lkj
        }
        sendHeader(status);

        try(FileInputStream fi= new FileInputStream(file)){
            int count;
            byte[] buffer = new byte[8000];
            while ((count = fi.read(buffer)) > 0)
            {
                dos.write(buffer, 0, count);
            }

            dos.flush();
        }catch (IOException ex){
            ex.printStackTrace();
        }
    }

    @Override
    public void run() {
        printFullMessage();
        MsgExtract();
        executeMethod();
    }
}



