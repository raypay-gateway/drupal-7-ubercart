<?php

/**
 * Class RayPay Handles RayPay Gateway transactions
 */
class RayPay {

    /**
     * @var bool Whether we should use nusoap library
     */
    protected $nusoap;

    /**
     * @var string User ID
     */
    protected $UserID;

    /**
     * @var string Marketing ID
     */
    protected $MarketingID;

    /**
     * @var bool EnableSandBox
     */
    protected $EnableSandBox;

    /**
     * @var int Amount in Tomans
     */
    protected $Amount;

    /**
     * @var string Description of the transaction
     */
    protected $Description;

    /**
     * @var string optional Email of the customer
     */
    protected $Email;

    /**
     * @var string optional Mobile number of the customer
     */
    protected $Mobile;

    /**
     * @var Callback URL of the shopping site
     */
    protected $CallbackURL;
    /**
     * @var bool Whether we should use iran switch or not (germany otherwise)
     */
    protected $Iran = False;

    /**
     * Class Constructor
     *
     * @param $UserID
     *   User ID
     * @param $MarketingID
     *   Marketing ID
     *  @param $EnableSandBox
     *   Enable SandBox
     * @param $Amount
     *   Amount In tomans
     * @param $Description
     *   Description of Payment
     * @param $CallbackURL
     *   Callback url of the Shopping site
     * @param string $Email
     *   Optional email of customer
     * @param string $Mobile
     *   Optional mobile number of the customer
     */
    function __construct($UserID, $MarketingID, $EnableSandBox, $Amount, $Description, $CallbackURL, $Email= '', $Mobile = '') {
        //Initialize variables
        $this->UserID = $UserID;
        $this->MarketingID = $MarketingID;
        $this->EnableSandBox = $EnableSandBox;
        $this->Amount = $Amount;
        $this->Description = $Description;
        $this->CallbackURL = $CallbackURL;
        $this->Email = $Email;
        $this->Mobile = $Mobile;
    }

    /**
     * Set the raypay switch to Iran or Germany or check it.
     *
     * @param $arg
     *   If True, the switch will be set to Iran
     *   If False, the switch will be set to Germany
     *   Otherwise, the switch status will be returned (True = Iran; False = Germany)
     * @return bool
     */
    public function isIran($arg = NULL) {
        if ($arg === TRUE) {
            $this->Iran = TRUE;
        }
        elseif ($arg === FALSE) {
            $this->Iran = FALSE;
        }
        else {
            return $this->Iran;
        }

        return TRUE;
    }

    /**
     * Request payment.
     *
     * @throws RayPayException
     *
     * @return String Payment Token
     */
    public function paymentRequest() {
        $invoice_id             = round(microtime(true) * 1000);
        $url = 'https://api.raypay.ir/raypay/api/v1/payment/pay';
        $param_request = array(
            'userID' => $this->UserID,
            'marketingID' => $this->MarketingID,
            'amount' => strval($this->Amount * 10),
            'enableSandBox' => $this->EnableSandBox,
            'invoiceID'    => strval($invoice_id),
            'desc' => $this->Description,
            'redirectUrl' => $this->CallbackURL . '?invoice_id=' . $invoice_id
        );
        $jsonData = json_encode($param_request);
        $ch = curl_init($url);
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'POST');
        curl_setopt($ch, CURLOPT_POSTFIELDS, $jsonData);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_HTTPHEADER, ['Content-Type: application/json']);

        $result = curl_exec($ch);
        $err = curl_error($ch);
        $result = json_decode($result, true, JSON_PRETTY_PRINT);
        curl_close($ch);

        if (($result['IsSuccess'] == true) && (isset($result['Data']))) {
            $token = $result['Data'];
        }
        else {
            throw new RayPayException(t('Can not request payment. !msg', array(
                '!msg' => $result['Message']
            )));
        }

        return $token;
    }

    /**
     * Verify transaction
     *
     * @param String $Authority
     *   The Authority number which is passed by RayPay to the callback page as QueryString
     *   If Null provided, it will be get from $_GET
     *
     * @throws RayPayException
     * @throws RayPayExceptionNok
     * @throws RayPayExceptionVerification
     *
     * @return int Reference ID of transaction if it is successful.
     */
    public function verifyTransaction() {
        // Validate $orderId (passed by raypay via QueryString in callback url)
        $invoice_id = $_GET['invoice_id'];
        if (is_null($invoice_id)) {
                throw new RayPayException(t('Error: Error returning Gateway '));
        }

        // Validated. Now Process verification
        $param_verify = $_POST;
        $jsonData = json_encode($param_verify);
        $url = 'https://api.raypay.ir/raypay/api/v1/payment/verify';
        $ch = curl_init($url);
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'POST');
        curl_setopt($ch, CURLOPT_POSTFIELDS, $jsonData);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_HTTPHEADER, ['Content-Type: application/json']);

        $result = curl_exec($ch);
        $err = curl_error($ch);
        curl_close($ch);
        $result = json_decode($result, true);

        if ($result['IsSuccess'] == true) {
            $state = $result['Data']['Status'];
            $verify_invoice_id = $result['Data']['InvoiceID'];
            $refid = $verify_invoice_id;

            if($state !== 1){
                throw new RayPayExceptionNok();
            }
        }
        else {
            throw new RayPayExceptionVerification(t('Transaction verification failed. RefID: %refid, Message: !message', array(
                '%refid'   => $invoice_id,
                '!message' => $result['Message'],
            )));
        }
        return $refid;
    }
}

/**
 * Custom Exception class for RayPay general errors.
 */
class RayPayException extends Exception {
}

/**
 * Custom Exception class
 * When RayPay sends Status=NOK to callback URL as query string.
 * In Sample PHP Code: Transaction canceled by user.
 */
class RayPayExceptionNok extends Exception {
}

/**
 * Custom Exception class
 * When RayPay verification transaction fails. (aka PaymentVerification() method
 * does not return 100)
 */
class RayPayExceptionVerification extends Exception {
}
