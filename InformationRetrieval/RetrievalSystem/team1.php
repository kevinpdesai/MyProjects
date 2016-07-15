<!DOCTYPE html>
<html>
	<head>
		<style>
			div {  
			  background: #eee;  
			  color: black;  
			}
			
			div.query {
				margin-top: 20px;  
				margin-right: 40%;  
				margin-bottom: 20px;  
				margin-left: 10%;
				position: relative;
			}
			
			div.perf {
				background: #b99;
				color: yellow;
				font-size: 18px;
			}
			
			div.padded {  
				padding-top: 25px;  
				padding-right: 20px;  
				padding-bottom: 20px;  
				padding-left: 20px;  
				margin-top: 20px;  
				margin-right: 40%;  
				margin-bottom: 10px;  
				margin-left: 10%;
			}
			

		</style>
	</head>
	<body>
		<form action="team1.php" method="POST">
			<div class="query">
				<div style="font-size:22px; position:relative; align:left; margin-left:5%; position:relative; top: 50%; ">Query&emsp;
					<input type="text" id="Q" name="query" size="27%" style="background:#fcc; height:30px; font-size:24px;" autocomplete="off"/>&emsp;
					<input type="submit" style="height: 30px; width: 90px; transform: translate(0, -12%)" value="Submit"/>
				</div>
			</div>
		</form>
		<?php
		if (!empty($_POST)) {
			echo "<div class=\"padded\">";
			echo "
			<script type=\"text/javascript\">document.getElementById(\"Q\").value = \"" . $_POST['query'] . "\";</script>";
			$q = "\"" . $_POST['query'] . "\"";
			//enable the following on windows
			$cmd = "binary \"Data\" \"resources\" " . $q . " 2>&1";
			// enable the following on linux
			//$cmd = "./binary \"/var/www/html/IR/Data\" \"/var/www/html/IR/resources\" " . $q . " 2>&1";
			exec($cmd, $arr);
			echo "<div class=\"perf\">Time taken to execute query on server is " . $arr[count($arr)-1] . "</div>";
			if (count($arr) != 21) {
				return;
			}
			for ($i = 0; $i < 20; $i = $i + 2) {
				echo "<p>" . $arr[$i] . "<br>";
				echo "<a href=\"" . $arr[$i+1] . "\">" . $arr[$i+1] . "</a></p>\n";
			}
			echo "</div>";
		}
		?>
		
	</body>
</html>
