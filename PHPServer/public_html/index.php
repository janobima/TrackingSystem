<?php
require_once 'firebaseLib.php';

// Firebase URL
$url = 'https://post-3fd40.firebaseio.com/';  //anon
//$url = 'https://lockingsystem-910cf.firebaseio.com/';

//Token from Firebase 
$token = 'HnsZcCwTpmUv0DVX1Ap9qdWn36WV7lndYYNcYsV4'; //anon
//$token = 'HaAVPxptpAet5QMm8Mkb0wFxbGygzFAspmuKRcSK'; //

//Parameters from the http GET
$myObj = new stdClass();

$myObj->ID = $_GET['ID'];
$myObj->Bat = $_GET['Bat'];
$myObj->Lon =  $_GET['Lon'];
$myObj->Lat = $_GET['Lat']; 
$myObj->Stat = $_GET['Stat']; 
$myObj->Tamp = $_GET['Tamp'];

//Firebase url structure
$firebasePath = '/Locks/' . $myObj ->ID  . "/";      

//Making calls
$fb = new fireBase($url, $token);

$getData = $fb->get($firebasePath);
print $getData; 

$decodedResponse = json_decode($getData);

if(!empty($decodedResponse->{'Stat'})){
    if (($myObj->Stat != $decodedResponse->{'Stat'}) ) {
        $myObj->Stat = $decodedResponse->{'Stat'}; 
    } 
}


$response = $fb->update($firebasePath, $myObj);
//$response = $fb->push($firebasePath, $myObj);

//Log who is connecting 
$line = date('Y-m-d H:i:s') . " - $_SERVER[REMOTE_ADDR]";
file_put_contents('../tmp/visitors.log', $line . PHP_EOL, FILE_APPEND);
//sleep(2);
?>
