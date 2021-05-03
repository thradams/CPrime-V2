

int main(){
    int i;
    
    if (1) /*try*/{
        
        {            
             i = 2;
        }

        /*throw*/ { i = 1; goto _catch_label1;};

        {int i = 0; if( i < 10){{
        } i = 3;}} i = 1;
    }
    else /*catch*/ _catch_label1:{
    }

   

}

