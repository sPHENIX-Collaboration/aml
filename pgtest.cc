 #include <et_PGConnection.h>

main()

{

  et_PGConnection *x =  et_PGConnection::instance("mlp", "phnxdb0.phenix.bnl.gov", "prdflist");

  x->addRecord( "CALIBDATA_P00-0000218628-0002.PRDFF", 218628, 2);

  delete x;
  return 0;
}
