package com.company;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

public class Converter
{
    public static void main(String args[]) throws IOException {
        File file= new File("src/line.jpg");
        FileOutputStream fos = new FileOutputStream("src/frame1.c");
        BufferedImage image = ImageIO.read(file);
        fos.write('{');
        int printed = 0;
        // Getting pixel color by position x and y
        for(int x=95; x<140; x++){
            for(int y=40; y<70; y++){
                int clr=  image.getRGB(x,y);
                int  red   = (clr & 0x00ff0000) >> 16;
                int  green = (clr & 0x0000ff00) >> 8;
                int  blue  =  clr & 0x000000ff;
                if(red >= 245 && green >= 245 && blue >= 245){
                    //System.out.print(" ");
                }else{
                    int x_ = x+260;
                    String out = " " + x_ + ", " + y + ",";
                    fos.write(out.getBytes());
                    printed ++;
                    if(printed == 20){
                        fos.write('\\');
                        fos.write('\n');
                        printed = 0;
                    }
                }
            }
        }
        String end = "" + -1 + "}";
        fos.write(end.getBytes());
        fos.close();
        System.out.println("done!");

    }
}

//{x, y,