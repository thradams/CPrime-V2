
int main() {

    if (1) /*try*/ {

        {
            int i = 1; if (i) {
                /*throw*/
                {
                    i = 1000; goto _catch_label1;
                };
            } i = 1000;
        }
    }
}
    else /*catch*/ _catch_label1:{

    }

}